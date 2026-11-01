/* SPDX-License-Identifier: MIT */
/* Same-TU resource adapter. No presentation or foreign callbacks. */
#ifndef CUI_BRIDGE_ICON_INTERNAL_H
#define CUI_BRIDGE_ICON_INTERNAL_H
#include "cui/cui_icon.h"
#include <limits.h>
_Static_assert(sizeof(cui_bridge_icon_handle_v1) == 8, "Icon handle ABI");
_Static_assert(sizeof(cui_bridge_icon_limits_v1) == 48, "Icon limits ABI");
_Static_assert(sizeof(cui_bridge_icon_info_v1) == 80, "Icon info ABI");
_Static_assert(sizeof(cui_bridge_icon_bitmap_v1) == 64, "Icon bitmap ABI");
_Static_assert(offsetof(cui_bridge_icon_limits_v1, max_svg_bytes) == 16, "Icon limits offsets");
_Static_assert(offsetof(cui_bridge_icon_info_v1, source_revision) == 48, "Icon revision offset");
_Static_assert(offsetof(cui_bridge_icon_info_v1, handle) == 64, "Icon identity offset");
_Static_assert(offsetof(cui_bridge_icon_bitmap_v1, required_bytes) == 32, "Icon bytes offset");
_Static_assert(offsetof(cui_bridge_icon_bitmap_v1, paint_x) == 48, "Icon bounds offset");
#define BRIDGE_ICON_SOURCE_BUDGET (UINT64_C(8) * 1024 * 1024)
#define BRIDGE_ICON_CACHE_BUDGET (UINT64_C(64) * 1024 * 1024)
typedef struct {
    char *path, *svg;
    uint32_t path_bytes, svg_bytes;
} bridge_icon_asset;
typedef struct {
    cui_icon_state state;
    cui_icon_limits limits;
    char *source;
    uint32_t source_bytes, source_kind, references, asset_count;
    uint64_t revision, native_source_reserved;
    bridge_icon_asset *assets;
} bridge_icon_owned;
typedef struct {
    bridge_icon_owned *value;
    uint32_t generation;
    bool retired;
} bridge_icon_slot;
struct bridge_icon_app {
    const void *owner;
    bridge_icon_slot *slots;
    uint32_t capacity;
    uint64_t cache_quota, svg_quota, copied_bytes;
    bool busy;
};
static cui_bridge_status bridge_icon_thread_check(cui_bridge_app *app) {
    return app->icons && (app->icons->owner != bridge_slider_owner() || app->icons->busy)
               ? CUI_BRIDGE_INVALID_STATE
               : CUI_BRIDGE_OK;
}
/* Validate known call ranges only; the opaque graph disjointness precondition
 * remains necessary. Empty ranges do not dereference their pointer. */
typedef struct {
    const void *pointer;
    size_t bytes;
} bridge_icon_range;
static bool bridge_icon_ranges(const bridge_icon_range *ranges, size_t count) {
    for (size_t i = 0; i < count; i++) {
        uintptr_t a = (uintptr_t)ranges[i].pointer;
        size_t an = ranges[i].bytes;
        if (an && (!ranges[i].pointer || an > UINTPTR_MAX - a))
            return false;
        if (!an)
            continue;
        for (size_t j = 0; j < i; j++) {
            uintptr_t b = (uintptr_t)ranges[j].pointer;
            size_t bn = ranges[j].bytes;
            if (bn && a < b + bn && b < a + an)
                return false;
        }
    }
    return true;
}
static bool bridge_icon_header(const void *p, uint32_t size) {
    bridge_icon_range range = {p, size};
    if (!bridge_icon_ranges(&range, 1))
        return false;
    uint32_t words[2];
    memcpy(words, p, sizeof words);
    return words[0] == size && words[1] == 1;
}
static cui_bridge_status bridge_icon_bytes(const char *s, uint32_t n, uint32_t cap) {
    bridge_icon_range range = {s, n};
    if (!bridge_icon_ranges(&range, 1))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (n > cap)
        return CUI_BRIDGE_CAPACITY;
    for (uint32_t i = 0; i < n;) {
        unsigned c = (unsigned char)s[i++];
        if (!c)
            return CUI_BRIDGE_INVALID_UTF8;
        if (c < 128)
            continue;
        unsigned extra;
        uint32_t scalar, minimum;
        if (c >= 0xc2 && c <= 0xdf) {
            extra = 1;
            scalar = c & 31u;
            minimum = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            extra = 2;
            scalar = c & 15u;
            minimum = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            extra = 3;
            scalar = c & 7u;
            minimum = 0x10000;
        } else
            return CUI_BRIDGE_INVALID_UTF8;
        if (extra > n - i)
            return CUI_BRIDGE_INVALID_UTF8;
        while (extra--) {
            unsigned b = (unsigned char)s[i++];
            if ((b & 0xc0) != 0x80)
                return CUI_BRIDGE_INVALID_UTF8;
            scalar = (scalar << 6) | (b & 63u);
        }
        if (scalar < minimum || scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
            return CUI_BRIDGE_INVALID_UTF8;
    }
    return CUI_BRIDGE_OK;
}
static char *bridge_icon_copy(const char *s, uint32_t n) {
    char *p = malloc((size_t)n + 1);
    if (!p)
        return NULL;
    if (n)
        memcpy(p, s, n);
    p[n] = 0;
    return p;
}
static cui_bridge_status bridge_icon_error(cui_icon_error e) {
    switch (e) {
    case CUI_ICON_OK:
        return CUI_BRIDGE_OK;
    case CUI_ICON_NOT_FOUND:
        return CUI_BRIDGE_EMPTY;
    case CUI_ICON_LIMIT:
        return CUI_BRIDGE_CAPACITY;
    case CUI_ICON_ALLOCATION:
        return CUI_BRIDGE_OUT_OF_MEMORY;
    default:
        return CUI_BRIDGE_INVALID_ARGUMENT;
    }
}
static cui_bridge_icon_limits_v1 bridge_icon_limits_copy(cui_icon_limits l) {
    return (cui_bridge_icon_limits_v1){.struct_size = sizeof(cui_bridge_icon_limits_v1),
                                       .schema_version = 1,
                                       .max_svg_bytes = l.max_svg_bytes,
                                       .max_pixels = l.max_pixels,
                                       .max_cache_bytes = l.max_cache_bytes,
                                       .max_versions = l.max_versions};
}
static cui_bridge_status bridge_icon_limits_read(const cui_bridge_icon_limits_v1 *p,
                                                 cui_icon_limits *out) {
    if (!bridge_icon_header(p, sizeof *p) || p->reserved[0] || p->reserved[1])
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_icon_limits d = cui_icon_default_limits();
    uint64_t svg = p->max_svg_bytes ? p->max_svg_bytes : d.max_svg_bytes;
    uint64_t pixels = p->max_pixels ? p->max_pixels : d.max_pixels;
    uint64_t cache = p->max_cache_bytes ? p->max_cache_bytes : d.max_cache_bytes;
    uint64_t versions = p->max_versions ? p->max_versions : d.max_versions;
    if (svg > 1024 * 1024 || pixels > 16 * 1024 * 1024 || cache > BRIDGE_ICON_CACHE_BUDGET ||
        versions > 128 || svg > SIZE_MAX || pixels > SIZE_MAX || cache > SIZE_MAX ||
        versions > SIZE_MAX)
        return CUI_BRIDGE_CAPACITY;
    *out = (cui_icon_limits){(size_t)svg, (size_t)pixels, (size_t)cache, (size_t)versions};
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_icon_get(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                         bridge_icon_owned **out) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!app->icons || !h.slot || !h.generation || h.slot > app->icons->capacity)
        return CUI_BRIDGE_STALE_HANDLE;
    bridge_icon_slot *s = &app->icons->slots[h.slot - 1];
    if (!s->value || s->generation != h.generation)
        return CUI_BRIDGE_STALE_HANDLE;
    *out = s->value;
    return CUI_BRIDGE_OK;
}
static uint64_t bridge_icon_owned_bytes(const bridge_icon_owned *v) {
    uint64_t n = (uint64_t)v->source_bytes + 1 + v->native_source_reserved;
    for (uint32_t i = 0; i < v->asset_count; i++)
        n += (uint64_t)v->assets[i].path_bytes + v->assets[i].svg_bytes + 2;
    return n;
}
static void bridge_icon_free(bridge_icon_owned *v) {
    if (!v)
        return;
    cui_icon_state_destroy(&v->state);
    free(v->source);
    for (uint32_t i = 0; i < v->asset_count; i++) {
        free(v->assets[i].path);
        free(v->assets[i].svg);
    }
    free(v->assets);
    free(v);
}
static void bridge_icon_app_destroy(cui_bridge_app *app) {
    bridge_icon_app *a = app->icons;
    if (!a)
        return;
    a->busy = true;
    for (uint32_t i = 0; i < a->capacity; i++)
        bridge_icon_free(a->slots[i].value);
    free(a->slots);
    free(a);
    app->icons = NULL;
}
static bool bridge_icon_loader(void *user, cui_str path, cui_str *svg) {
    bridge_icon_owned *v = user;
    for (uint32_t i = 0; i < v->asset_count; i++)
        if (path.length == v->assets[i].path_bytes &&
            !memcmp(path.data, v->assets[i].path, path.length)) {
            *svg = (cui_str){v->assets[i].svg, v->assets[i].svg_bytes};
            return true;
        }
    return false;
}
uint32_t cui_bridge_icon_versions(void) {
    return BRIDGE_SLIDER_SUPPORTED ? 1u : 0u;
}
uint32_t cui_bridge_icon_record_size_v1(uint32_t kind) {
    switch (kind) {
    case 3:
        return sizeof(cui_bridge_icon_limits_v1);
    case 4:
        return sizeof(cui_bridge_icon_info_v1);
    case 5:
        return sizeof(cui_bridge_icon_bitmap_v1);
    default:
        return 0;
    }
}
cui_bridge_status cui_bridge_icon_default_limits_v1(cui_bridge_icon_limits_v1 *out) {
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = bridge_icon_limits_copy(cui_icon_default_limits());
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_create_v1(cui_bridge_app *app,
                                            const cui_bridge_icon_limits_v1 *limits,
                                            cui_bridge_icon_handle_v1 *out) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!BRIDGE_SLIDER_SUPPORTED)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_icon_range ranges[] = {{limits, sizeof *limits}, {out, sizeof *out}};
    if (!bridge_icon_ranges(ranges, 2))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_icon_limits l;
    status = bridge_icon_limits_read(limits, &l);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_app *a = app->icons;
    if (a && (l.max_cache_bytes > BRIDGE_ICON_CACHE_BUDGET - a->cache_quota ||
              (uint64_t)l.max_svg_bytes + 1 >
                  BRIDGE_ICON_SOURCE_BUDGET - a->svg_quota - a->copied_bytes))
        return CUI_BRIDGE_CAPACITY;
    bool fresh = !a;
    if (fresh) {
        a = calloc(1, sizeof *a);
        if (!a)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        a->owner = bridge_slider_owner();
    }
    uint32_t i = 0;
    for (; i < a->capacity; i++)
        if (!a->slots[i].value && !a->slots[i].retired)
            break;
    if (i == a->capacity) {
        if (a->capacity == 128) {
            if (fresh)
                free(a);
            return CUI_BRIDGE_CAPACITY;
        }
        uint32_t count = a->capacity ? a->capacity * 2 : 8;
        bridge_icon_slot *slots = calloc(count, sizeof *slots);
        if (!slots) {
            if (fresh)
                free(a);
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        if (a->capacity)
            memcpy(slots, a->slots, a->capacity * sizeof *slots);
        free(a->slots);
        a->slots = slots;
        a->capacity = count;
    }
    a->busy = true;
    bridge_icon_owned *v = calloc(1, sizeof *v);
    if (v) {
        cui_icon_state_init(&v->state);
        v->limits = l;
        v->revision = 1;
        v->source = bridge_icon_copy(NULL, 0);
    }
    if (!v || !v->source || !cui_icon_set_limits(&v->state, l)) {
        bridge_icon_free(v);
        a->busy = false;
        if (fresh) {
            free(a->slots);
            free(a);
        }
        return CUI_BRIDGE_OUT_OF_MEMORY;
    }
    bridge_icon_slot *slot = &a->slots[i];
    if (!slot->generation)
        slot->generation = 1;
    slot->value = v;
    a->cache_quota += l.max_cache_bytes;
    a->svg_quota += l.max_svg_bytes;
    a->copied_bytes++;
    a->busy = false;
    app->icons = a;
    *out = (cui_bridge_icon_handle_v1){i + 1, slot->generation};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_destroy_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (v->references)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_icon_app *a = app->icons;
    bridge_icon_slot *s = &a->slots[h.slot - 1];
    a->busy = true;
    a->cache_quota -= v->limits.max_cache_bytes;
    a->svg_quota -= v->limits.max_svg_bytes;
    a->copied_bytes -= bridge_icon_owned_bytes(v);
    s->value = NULL;
    if (s->generation == UINT32_MAX)
        s->retired = true;
    else
        s->generation++;
    bridge_icon_free(v);
    a->busy = false;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_get_info_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                              cui_bridge_icon_info_v1 *out) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = (cui_bridge_icon_info_v1){.struct_size = sizeof *out,
                                     .schema_version = 1,
                                     .icon_error = v->state.error,
                                     .reference_count = v->references,
                                     .parse_count = v->state.parse_count,
                                     .raster_count = v->state.raster_count,
                                     .cache_bytes = v->state.cache_bytes,
                                     .cache_versions = v->state.cache_versions,
                                     .source_revision = v->revision,
                                     .source_kind = v->source_kind,
                                     .handle = h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_get_limits_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                cui_bridge_icon_limits_v1 *out) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = bridge_icon_limits_copy(v->limits);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_set_limits_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                const cui_bridge_icon_limits_v1 *in) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_icon_limits l;
    status = bridge_icon_limits_read(in, &l);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_app *a = app->icons;
    if (l.max_cache_bytes >
            BRIDGE_ICON_CACHE_BUDGET - (a->cache_quota - v->limits.max_cache_bytes) ||
        l.max_svg_bytes >
            BRIDGE_ICON_SOURCE_BUDGET - (a->svg_quota - v->limits.max_svg_bytes) - a->copied_bytes)
        return CUI_BRIDGE_CAPACITY;
    a->busy = true;
    bool ok = cui_icon_set_limits(&v->state, l);
    a->busy = false;
    if (!ok)
        return bridge_icon_error(v->state.error);
    a->cache_quota = a->cache_quota - v->limits.max_cache_bytes + l.max_cache_bytes;
    a->svg_quota = a->svg_quota - v->limits.max_svg_bytes + l.max_svg_bytes;
    v->limits = l;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_set_source_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                uint32_t kind, const char *text, uint32_t n) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (kind > 3 || (kind == 0 && n))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_icon_bytes(text, n, kind == 3 ? (uint32_t)v->limits.max_svg_bytes : 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_app *a = app->icons;
    /* C NAME stores a canonical path; conservatively reserve its published
     * maximum without duplicating alias/catalog normalization. C limits never
     * shrink an already-owned source, so this reservation survives limit edits. */
    uint64_t native_bytes = kind == CUI_BRIDGE_ICON_NAME_V1 ? 4097 : (uint64_t)n + 1;
    uint64_t new_bytes = (uint64_t)n + 1 + native_bytes;
    if (v->revision == UINT64_MAX ||
        new_bytes > BRIDGE_ICON_SOURCE_BUDGET - a->svg_quota - a->copied_bytes)
        return CUI_BRIDGE_CAPACITY;
    a->busy = true;
    char *copy = bridge_icon_copy(text, n);
    if (!copy) {
        a->busy = false;
        return CUI_BRIDGE_OUT_OF_MEMORY;
    }
    cui_str source = {copy, n};
    bool ok;
    if (kind == 0)
        ok = cui_icon_set_empty(&v->state);
    else if (kind == 1)
        ok = cui_icon_set_name(&v->state, source, bridge_icon_loader, v);
    else if (kind == 2)
        ok = cui_icon_set_path(&v->state, source, bridge_icon_loader, v);
    else
        ok = cui_icon_set_data(&v->state, source);
    if (!ok) {
        free(copy);
        a->busy = false;
        return bridge_icon_error(v->state.error);
    }
    a->copied_bytes = a->copied_bytes - v->source_bytes - 1 - v->native_source_reserved + new_bytes;
    v->native_source_reserved = native_bytes;
    free(v->source);
    v->source = copy;
    v->source_bytes = n;
    v->source_kind = kind;
    v->revision++;
    a->busy = false;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_icon_string(cui_str s, char *out, uint32_t cap,
                                            uint32_t *required) {
    bridge_icon_range ranges[] = {{s.data, s.length}, {out, cap}, {required, sizeof *required}};
    if (s.length >= UINT32_MAX || !bridge_icon_ranges(ranges, 3))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    uint32_t n = (uint32_t)s.length + 1;
    if (cap < n) {
        *required = n;
        return CUI_BRIDGE_CAPACITY;
    }
    if (!out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (s.length)
        memcpy(out, s.data, s.length);
    out[s.length] = 0;
    *required = n;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_get_source_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                uint32_t *kind, char *out, uint32_t cap,
                                                uint32_t *required) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{v->source, (size_t)v->source_bytes + 1},
                                  {kind, sizeof *kind},
                                  {out, cap},
                                  {required, sizeof *required}};
    if (!bridge_icon_ranges(ranges, 4))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_icon_string((cui_str){v->source, v->source_bytes}, out, cap, required);
    if (status == CUI_BRIDGE_OK)
        *kind = v->source_kind;
    return status;
}
cui_bridge_status cui_bridge_icon_reload_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (v->revision == UINT64_MAX)
        return CUI_BRIDGE_CAPACITY;
    cui_icon_reload(&v->state);
    v->revision++;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_collect_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    app->icons->busy = true;
    cui_icon_state_collect(&v->state);
    app->icons->busy = false;
    return CUI_BRIDGE_OK;
}
static uint32_t bridge_icon_asset_index(const bridge_icon_owned *v, const char *path, uint32_t n) {
    for (uint32_t i = 0; i < v->asset_count; i++)
        if (v->assets[i].path_bytes == n && !memcmp(v->assets[i].path, path, n))
            return i;
    return UINT32_MAX;
}
cui_bridge_status cui_bridge_icon_asset_put_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                               const char *path, uint32_t pn, const char *svg,
                                               uint32_t sn) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!pn || !sn)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_icon_bytes(path, pn, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_icon_bytes(svg, sn, (uint32_t)v->limits.max_svg_bytes);
    if (status != CUI_BRIDGE_OK)
        return status;
    uint32_t index = bridge_icon_asset_index(v, path, pn);
    bool adding = index == UINT32_MAX;
    bridge_icon_app *a = app->icons;
    uint64_t bytes = (uint64_t)pn + sn + 2;
    if ((adding && v->asset_count == 128) || v->revision == UINT64_MAX ||
        bytes > BRIDGE_ICON_SOURCE_BUDGET - a->svg_quota - a->copied_bytes)
        return CUI_BRIDGE_CAPACITY;
    a->busy = true;
    char *pc = bridge_icon_copy(path, pn), *sc = bridge_icon_copy(svg, sn);
    bridge_icon_asset *entries = v->assets;
    if (pc && sc && adding)
        entries = malloc((v->asset_count + 1) * sizeof *entries);
    if (!pc || !sc || !entries) {
        free(pc);
        free(sc);
        a->busy = false;
        return CUI_BRIDGE_OUT_OF_MEMORY;
    }
    if (adding) {
        if (v->asset_count)
            memcpy(entries, v->assets, v->asset_count * sizeof *entries);
        free(v->assets);
        v->assets = entries;
        index = v->asset_count++;
    } else {
        a->copied_bytes -= (uint64_t)v->assets[index].path_bytes + v->assets[index].svg_bytes + 2;
        free(v->assets[index].path);
        free(v->assets[index].svg);
    }
    v->assets[index] = (bridge_icon_asset){pc, sc, pn, sn};
    a->copied_bytes += bytes;
    v->revision++;
    cui_icon_reload(&v->state);
    a->busy = false;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_asset_remove_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                  const char *path, uint32_t pn) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_icon_bytes(path, pn, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!pn)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    uint32_t i = bridge_icon_asset_index(v, path, pn);
    if (i == UINT32_MAX)
        return CUI_BRIDGE_EMPTY;
    if (v->revision == UINT64_MAX)
        return CUI_BRIDGE_CAPACITY;
    app->icons->busy = true;
    app->icons->copied_bytes -= (uint64_t)v->assets[i].path_bytes + v->assets[i].svg_bytes + 2;
    free(v->assets[i].path);
    free(v->assets[i].svg);
    memmove(v->assets + i, v->assets + i + 1, (v->asset_count - i - 1) * sizeof *v->assets);
    v->asset_count--;
    v->revision++;
    cui_icon_reload(&v->state);
    app->icons->busy = false;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_asset_clear_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!v->asset_count)
        return CUI_BRIDGE_OK;
    if (v->revision == UINT64_MAX)
        return CUI_BRIDGE_CAPACITY;
    app->icons->busy = true;
    for (uint32_t i = 0; i < v->asset_count; i++) {
        app->icons->copied_bytes -= (uint64_t)v->assets[i].path_bytes + v->assets[i].svg_bytes + 2;
        free(v->assets[i].path);
        free(v->assets[i].svg);
    }
    free(v->assets);
    v->assets = NULL;
    v->asset_count = 0;
    v->revision++;
    cui_icon_reload(&v->state);
    app->icons->busy = false;
    return CUI_BRIDGE_OK;
}
uint32_t cui_bridge_icon_catalog_count_v1(void) {
    return (uint32_t)cui_icon_catalog_count();
}
cui_bridge_status cui_bridge_icon_catalog_entry_v1(uint32_t index, uint32_t *is_default, char *name,
                                                   uint32_t nc, uint32_t *nr, char *path,
                                                   uint32_t pc, uint32_t *pr) {
    bridge_icon_range ranges[] = {{is_default, sizeof *is_default},
                                  {name, nc},
                                  {nr, sizeof *nr},
                                  {path, pc},
                                  {pr, sizeof *pr}};
    if (index >= cui_icon_catalog_count() || !bridge_icon_ranges(ranges, 5))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_str n = cui_icon_catalog_name(index), p = cui_icon_catalog_path(index);
    uint32_t nn = (uint32_t)n.length + 1, pn = (uint32_t)p.length + 1;
    if (nc < nn || pc < pn) {
        *nr = nn;
        *pr = pn;
        return CUI_BRIDGE_CAPACITY;
    }
    if (!name || !path)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    memcpy(name, n.data, n.length);
    name[n.length] = 0;
    memcpy(path, p.data, p.length);
    path[p.length] = 0;
    *nr = nn;
    *pr = pn;
    *is_default = cui_icon_catalog_default(index) ? 1u : 0u;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_query_v1(const char *name, uint32_t n, uint32_t *out) {
    bridge_icon_range ranges[] = {{name, n}, {out, sizeof *out}};
    if (!bridge_icon_ranges(ranges, 2))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_icon_bytes(name, n, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    *out = (uint32_t)cui_icon_query((cui_str){name, n});
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_asset_copy_v1(const char *name, uint32_t n, char *out,
                                                uint32_t cap, uint32_t *required) {
    bridge_icon_range ranges[] = {{name, n}, {out, cap}, {required, sizeof *required}};
    if (!bridge_icon_ranges(ranges, 3))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_icon_bytes(name, n, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_str svg;
    if (!cui_icon_asset((cui_str){name, n}, &svg))
        return CUI_BRIDGE_EMPTY;
    return bridge_icon_string(svg, out, cap, required);
}
cui_bridge_status cui_bridge_icon_prepare_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                             float width, float height, const float transform[6],
                                             uint32_t *error) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{transform, 6 * sizeof *transform}, {error, sizeof *error}};
    if (!bridge_icon_ranges(ranges, 2))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_icon_transform t = {transform[0], transform[1], transform[2],
                            transform[3], transform[4], transform[5]};
    app->icons->busy = true;
    cui_icon_error e = cui_icon_prepare(&v->state, width, height, t);
    app->icons->busy = false;
    *error = (uint32_t)e;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_icon_bitmap_copy_v1(cui_bridge_app *app, cui_bridge_icon_handle_v1 h,
                                                 cui_bridge_icon_bitmap_v1 *out,
                                                 unsigned char *pixels, uint32_t cap,
                                                 uint32_t *required) {
    bridge_icon_owned *v;
    cui_bridge_status status = bridge_icon_get(app, h, &v);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{out, sizeof *out}, {pixels, cap}, {required, sizeof *required}};
    if (!bridge_icon_ranges(ranges, 3) || !bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bitmap b = cui_icon_state_bitmap(&v->state);
    cui_bridge_icon_bitmap_v1 result = {.struct_size = sizeof result, .schema_version = 1};
    uint32_t n = 0;
    if (cui_bitmap_valid(b)) {
        uint64_t row = (uint64_t)b.width * 4, bytes = row * b.height;
        if (row > UINT32_MAX || bytes > UINT32_MAX)
            return CUI_BRIDGE_CAPACITY;
        n = (uint32_t)bytes;
        result.width = b.width;
        result.height = b.height;
        result.stride = (uint32_t)row;
        result.required_bytes = bytes;
        result.revision = b.revision;
        cui_rect p = cui_icon_state_paint_bounds(&v->state);
        result.paint_x = p.x;
        result.paint_y = p.y;
        result.paint_width = p.w;
        result.paint_height = p.h;
    }
    bridge_icon_range all[] = {{out, sizeof *out},
                               {pixels, cap},
                               {required, sizeof *required},
                               {b.pixels, n ? (size_t)b.stride * b.height : 0}};
    if (!bridge_icon_ranges(all, 4))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (cap < n) {
        *required = n;
        return CUI_BRIDGE_CAPACITY;
    }
    if (n && !pixels)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    for (uint32_t y = 0; y < result.height; y++)
        memcpy(pixels + (size_t)y * result.stride, b.pixels + (size_t)y * b.stride, result.stride);
    *out = result;
    *required = n;
    return CUI_BRIDGE_OK;
}
#undef BRIDGE_ICON_SOURCE_BUDGET
#undef BRIDGE_ICON_CACHE_BUDGET
#endif
