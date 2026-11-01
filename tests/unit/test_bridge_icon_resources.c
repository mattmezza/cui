/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#if defined(CUI_ICON_LINK_WRAP_MALLOC)
/* Optional GNU-linker fixture: actually fail the ordinary Icon implementation's
 * malloc, not a simulated Icon result. Never enabled in production. */
void *__real_malloc(size_t);
static unsigned native_fail_after;
void *__wrap_malloc(size_t n) {
    if (native_fail_after && --native_fail_after == 0)
        return NULL;
    return __real_malloc(n);
}
#endif
static unsigned icon_allocations, icon_fail_at;
static void *icon_test_malloc(size_t size) {
    if (++icon_allocations == icon_fail_at)
        return NULL;
    return malloc(size);
}
static void *icon_test_calloc(size_t count, size_t size) {
    if (++icon_allocations == icon_fail_at)
        return NULL;
    return calloc(count, size);
}
#define malloc icon_test_malloc
#define calloc icon_test_calloc
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#undef malloc
#undef calloc
#include <assert.h>
#include <stdio.h>

static const char square[] = "<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'><path "
                             "d='M0 0H10V10H0Z'/></svg>";
static cui_bridge_icon_limits_v1 limits(void) {
    return (cui_bridge_icon_limits_v1){.struct_size = sizeof(cui_bridge_icon_limits_v1),
                                       .schema_version = 1};
}
static cui_bridge_icon_info_v1 info(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    cui_bridge_icon_info_v1 out = {.struct_size = sizeof out, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, h, &out) == CUI_BRIDGE_OK);
    return out;
}
static cui_bridge_app *create(void) {
    cui_bridge_app *app = NULL;
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = 1,
                            .width = 320,
                            .height = 240,
                            .max_nodes = 32,
                            .max_events = 32};
    cui_bridge_status status;
    app = cui_bridge_create(&o, &status);
    assert(status == CUI_BRIDGE_OK);
    assert(app && !app->icons);
    return app;
}
static void source(cui_bridge_app *a, cui_bridge_icon_handle_v1 h, uint32_t expected,
                   const char *text) {
    char output[1024];
    uint32_t kind = 99, required = 99;
    assert(cui_bridge_icon_get_source_v1(a, h, &kind, output, sizeof output, &required) ==
           CUI_BRIDGE_OK);
    assert(kind == expected && required == strlen(text) + 1 && !strcmp(output, text));
}
static uint32_t prepare(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    float t[] = {1, 0, 0, 1, 0, 0};
    uint32_t error = 99;
    assert(cui_bridge_icon_prepare_v1(app, h, 16, 16, t, &error) == CUI_BRIDGE_OK);
    return error;
}
static void public_records(void) {
    assert(cui_bridge_icon_versions() == 1);
    assert(cui_bridge_icon_record_size_v1(3) == 48);
    assert(cui_bridge_icon_record_size_v1(4) == 80 && cui_bridge_icon_record_size_v1(5) == 64);
    for (uint32_t k = 0; k < 10; k++)
        if (k != 3 && k != 4 && k != 5)
            assert(cui_bridge_icon_record_size_v1(k) == 0);
    cui_bridge_icon_limits_v1 d = limits();
    assert(cui_bridge_icon_default_limits_v1(&d) == CUI_BRIDGE_OK);
    assert(d.max_svg_bytes == 256 * 1024 && d.max_pixels == 1024 * 1024 &&
           d.max_cache_bytes == 8 * 1024 * 1024 && d.max_versions == 16);
    uint32_t count = cui_bridge_icon_catalog_count_v1();
    assert(count == cui_icon_catalog_count() && count > 1000);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t def = 99, nr = 0, pr = 0;
        char n[128], p[160];
        memset(n, 0xcc, sizeof n);
        memset(p, 0xdd, sizeof p);
        assert(cui_bridge_icon_catalog_entry_v1(i, &def, n, 0, &nr, p, 0, &pr) ==
               CUI_BRIDGE_CAPACITY);
        assert(def == 99 && (unsigned char)n[0] == 0xcc && (unsigned char)p[0] == 0xdd);
        assert(cui_bridge_icon_catalog_entry_v1(i, &def, n, sizeof n, &nr, p, sizeof p, &pr) ==
               CUI_BRIDGE_OK);
        assert(!strcmp(n, cui_icon_catalog_name(i).data) &&
               !strcmp(p, cui_icon_catalog_path(i).data));
        assert(def == (uint32_t)cui_icon_catalog_default(i) && nr == strlen(n) + 1 &&
               pr == strlen(p) + 1);
    }
    uint32_t availability = 99;
    assert(cui_bridge_icon_query_v1("trash-2", 7, &availability) == CUI_BRIDGE_OK);
    assert(availability == CUI_ICON_EMBEDDED);
    assert(cui_bridge_icon_query_v1("clock-3", 7, &availability) == CUI_BRIDGE_OK);
    assert(availability == (uint32_t)cui_icon_query(cui_string("clock-3")));
    assert(cui_bridge_icon_query_v1("nonesuch", 8, &availability) == CUI_BRIDGE_OK &&
           availability == 0);
    const char invalid[] = {(char)0xff};
    availability = 77;
    assert(cui_bridge_icon_query_v1(invalid, 1, &availability) == CUI_BRIDGE_INVALID_UTF8 &&
           availability == 77);
    uint32_t required = 99;
    assert(cui_bridge_icon_asset_copy_v1("nonesuch", 8, NULL, 0, &required) == CUI_BRIDGE_EMPTY &&
           required == 99);
    assert(cui_bridge_icon_asset_copy_v1("check", 5, NULL, 0, &required) == CUI_BRIDGE_CAPACITY &&
           required > 10);
    char *svg = malloc(required);
    assert(svg);
    assert(cui_bridge_icon_asset_copy_v1("check", 5, svg, required, &required) == CUI_BRIDGE_OK);
    cui_str original;
    assert(cui_icon_asset(cui_string("check"), &original));
    assert(strlen(svg) == original.length && !memcmp(svg, original.data, original.length));
    free(svg);
}
static void resource_lifetime(void) {
    cui_bridge_app *app = create();
    cui_bridge_icon_limits_v1 l = limits();
    cui_bridge_icon_handle_v1 h;
    assert(cui_bridge_icon_create_v1(app, &l, &h) == CUI_BRIDGE_OK && h.slot && h.generation);
    l = limits();
    assert(cui_bridge_icon_get_limits_v1(app, h, &l) == CUI_BRIDGE_OK &&
           l.max_cache_bytes == 8 * 1024 * 1024);
    assert(info(app, h).source_revision == 1 && info(app, h).reference_count == 0);
    source(app, h, 0, "");
    char bytes[sizeof square];
    memcpy(bytes, square, sizeof bytes);
    assert(cui_bridge_icon_set_source_v1(app, h, 3, bytes, sizeof bytes - 1) == CUI_BRIDGE_OK);
    memset(bytes, 'x', sizeof bytes);
    source(app, h, 3, square);
    assert(prepare(app, h) == CUI_ICON_OK);
    cui_bridge_icon_bitmap_v1 b = {.struct_size = sizeof b, .schema_version = 1}, before = b;
    uint32_t required = 0;
    assert(cui_bridge_icon_bitmap_copy_v1(app, h, &b, NULL, 0, &required) == CUI_BRIDGE_CAPACITY);
    assert(!memcmp(&b, &before, sizeof b) && required > 0 && required < 4096);
    unsigned char pixels[4096];
    memset(pixels, 0xa7, sizeof pixels);
    assert(cui_bridge_icon_bitmap_copy_v1(app, h, &b, pixels, sizeof pixels, &required) ==
           CUI_BRIDGE_OK);
    assert(b.width == 16 && b.height == 16 && b.stride == 64 && b.required_bytes == required &&
           b.revision);
    cui_bitmap native = cui_icon_state_bitmap(&app->icons->slots[h.slot - 1].value->state);
    assert(!memcmp(pixels, native.pixels, required));
    cui_icon_state independent = {0};
    assert(cui_icon_set_data(&independent, cui_string(square)));
    assert(cui_icon_prepare(&independent, 16, 16, cui_icon_identity()) == CUI_ICON_OK);
    cui_bitmap reference = cui_icon_state_bitmap(&independent);
    assert(reference.width == b.width && reference.height == b.height &&
           reference.stride == b.stride);
    assert(!memcmp(reference.pixels, pixels, required));
    cui_icon_state_destroy(&independent);
    unsigned char *old = malloc(required);
    assert(old);
    memcpy(old, native.pixels, required);
    assert(cui_bridge_icon_set_source_v1(app, h, 2, "missing.svg", 11) == CUI_BRIDGE_OK);
    assert(!memcmp(old, native.pixels, required));
    free(old);
    assert(prepare(app, h) == CUI_ICON_NOT_FOUND);
    source(app, h, 2, "missing.svg");
    assert(cui_bridge_icon_set_source_v1(app, h, 1, "unknown-name", 12) == CUI_BRIDGE_EMPTY);
    source(app, h, 2, "missing.svg");
    assert(info(app, h).icon_error == CUI_ICON_NOT_FOUND);
    assert(cui_bridge_icon_set_source_v1(app, h, 1, "", 0) == CUI_BRIDGE_EMPTY);
    source(app, h, 2, "missing.svg");
    assert(cui_bridge_icon_set_source_v1(app, h, 2, "", 0) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK);
    assert(cui_bridge_icon_set_source_v1(app, h, 3, "", 0) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK);
    assert(cui_bridge_icon_set_source_v1(app, h, 0, NULL, 0) == CUI_BRIDGE_OK);
    source(app, h, 0, "");
    b = (cui_bridge_icon_bitmap_v1){.struct_size = sizeof b, .schema_version = 1};
    assert(prepare(app, h) == CUI_ICON_OK &&
           cui_bridge_icon_bitmap_copy_v1(app, h, &b, NULL, 0, &required) == CUI_BRIDGE_OK);
    assert(!b.width && !b.height && !b.stride && !b.required_bytes && !b.revision && !required);
    assert(cui_bridge_icon_set_source_v1(app, h, 3, square, sizeof square - 1) == CUI_BRIDGE_OK);
    assert(prepare(app, h) == CUI_ICON_OK);
    uint64_t revision = info(app, h).source_revision;
    assert(cui_bridge_icon_reload_v1(app, h) == CUI_BRIDGE_OK &&
           info(app, h).source_revision == revision + 1);
    assert(cui_bridge_icon_collect_v1(app, h) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, h) == CUI_BRIDGE_OK);
    cui_bridge_icon_info_v1 output = {.struct_size = sizeof output, .schema_version = 1},
                            saved = output;
    assert(cui_bridge_icon_get_info_v1(app, h, &output) == CUI_BRIDGE_STALE_HANDLE &&
           !memcmp(&output, &saved, sizeof output));
    cui_bridge_icon_handle_v1 next;
    assert(cui_bridge_icon_create_v1(app, &l, &next) == CUI_BRIDGE_OK);
    assert(next.slot == h.slot && next.generation != h.generation);
    cui_bridge_destroy(app);
}
static void maps_and_failures(void) {
    cui_bridge_app *app = create();
    cui_bridge_icon_limits_v1 l = limits();
    cui_bridge_icon_handle_v1 h;
    assert(cui_bridge_icon_create_v1(app, &l, &h) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_asset_put_v1(app, h, "custom.svg", 10, square, sizeof square - 1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, h, 2, "custom.svg", 10) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK);
    uint64_t rev = info(app, h).source_revision;
    unsigned base = icon_allocations;
    icon_fail_at = base + 1;
    assert(cui_bridge_icon_set_source_v1(app, h, 3, square, sizeof square - 1) ==
           CUI_BRIDGE_OUT_OF_MEMORY);
    icon_fail_at = 0;
    assert(info(app, h).source_revision == rev);
    source(app, h, 2, "custom.svg");
#if defined(CUI_ICON_LINK_WRAP_MALLOC)
    native_fail_after = 2; /* bridge request copy succeeds; native source copy fails */
    assert(cui_bridge_icon_set_source_v1(app, h, 3, square, sizeof square - 1) ==
           CUI_BRIDGE_OUT_OF_MEMORY);
    assert(native_fail_after == 0 && info(app, h).icon_error == CUI_ICON_ALLOCATION);
    assert(info(app, h).source_revision == rev);
    source(app, h, 2, "custom.svg");
    assert(prepare(app, h) == CUI_ICON_OK);
#endif
    for (unsigned fail = 1; fail <= 2; fail++) {
        icon_fail_at = icon_allocations + fail;
        assert(cui_bridge_icon_asset_put_v1(app, h, "custom.svg", 10, "bad", 3) ==
               CUI_BRIDGE_OUT_OF_MEMORY);
        icon_fail_at = 0;
        assert(info(app, h).source_revision == rev && prepare(app, h) == CUI_ICON_OK);
    }
    assert(cui_bridge_icon_asset_put_v1(app, h, "custom.svg", 10, "bad", 3) == CUI_BRIDGE_OK);
    assert(prepare(app, h) == CUI_ICON_PARSE);
    assert(cui_bridge_icon_asset_remove_v1(app, h, "absent", 6) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_icon_asset_clear_v1(app, h) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_NOT_FOUND);
    assert(cui_bridge_icon_asset_put_v1(app, h, "icons/check.svg", 15, square, sizeof square - 1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, h, 1, "check", 5) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK);
    assert(cui_bridge_icon_asset_remove_v1(app, h, "icons/check.svg", 15) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK); /* embedded fallback */
    l = limits();
    l.max_pixels = 1;
    assert(cui_bridge_icon_set_limits_v1(app, h, &l) == CUI_BRIDGE_OK);
    assert(prepare(app, h) == CUI_ICON_LIMIT);
    l.max_pixels = 0;
    assert(cui_bridge_icon_set_limits_v1(app, h, &l) == CUI_BRIDGE_OK &&
           prepare(app, h) == CUI_ICON_OK);
    app->stepping = true;
    uint32_t error = 77;
    float transform[] = {1, 0, 0, 1, 0, 0};
    assert(cui_bridge_icon_prepare_v1(app, h, 16, 16, transform, &error) ==
               CUI_BRIDGE_INVALID_STATE &&
           error == 77);
    app->stepping = false;
    app->icons->busy = true;
    assert(cui_bridge_request_close(app) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_destroy(app);
    app->icons->busy = false;
    app->icons->slots[h.slot - 1].value->references = 1;
    assert(cui_bridge_icon_destroy_v1(app, h) == CUI_BRIDGE_INVALID_STATE);
    app->icons->slots[h.slot - 1].value->references = 0;
    assert(cui_bridge_request_close(app) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_collect_v1(app, h) == CUI_BRIDGE_CLOSED);
    cui_bridge_destroy(app);
}
static void quotas_and_lazy(void) {
    for (unsigned fail = 1; fail <= 4; fail++) {
        cui_bridge_app *a = create();
        cui_bridge_icon_limits_v1 l = limits();
        cui_bridge_icon_handle_v1 h = {99, 99};
        icon_fail_at = icon_allocations + fail;
        assert(cui_bridge_icon_create_v1(a, &l, &h) == CUI_BRIDGE_OUT_OF_MEMORY);
        icon_fail_at = 0;
        assert(!a->icons && h.slot == 99 && h.generation == 99);
        cui_bridge_destroy(a);
    }
    cui_bridge_app *a = create();
    cui_bridge_icon_limits_v1 l = limits();
    cui_bridge_icon_handle_v1 h[9];
    for (unsigned i = 0; i < 8; i++)
        assert(cui_bridge_icon_create_v1(a, &l, &h[i]) == CUI_BRIDGE_OK);
    h[8] = (cui_bridge_icon_handle_v1){99, 99};
    assert(cui_bridge_icon_create_v1(a, &l, &h[8]) == CUI_BRIDGE_CAPACITY && h[8].slot == 99);
    l.max_svg_bytes = UINT64_MAX;
    assert(cui_bridge_icon_set_limits_v1(a, h[0], &l) == CUI_BRIDGE_CAPACITY);
    l = limits();
    l.reserved[0] = 1;
    assert(cui_bridge_icon_set_limits_v1(a, h[0], &l) == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_destroy(a);
}
static void retained_source_budget(void) {
    const uint32_t mib = 1024 * 1024;
    char *data = malloc(mib);
    assert(data);
    memset(data, ' ', mib);
    cui_bridge_app *a = create();
    cui_bridge_icon_handle_v1 h[4];
    cui_bridge_icon_limits_v1 l = limits();
    l.max_svg_bytes = mib;
    l.max_cache_bytes = 1;
    for (unsigned i = 0; i < 3; i++) {
        assert(cui_bridge_icon_create_v1(a, &l, &h[i]) == CUI_BRIDGE_OK);
        assert(cui_bridge_icon_set_source_v1(a, h[i], 3, data, mib) == CUI_BRIDGE_OK);
        cui_bridge_icon_limits_v1 tiny = l;
        tiny.max_svg_bytes = 1;
        assert(cui_bridge_icon_set_limits_v1(a, h[i], &tiny) == CUI_BRIDGE_OK);
        assert(a->icons->slots[h[i].slot - 1].value->native_source_reserved == mib + 1);
    }
    assert(a->icons->copied_bytes == 3 * ((uint64_t)mib * 2 + 2) && a->icons->svg_quota == 3);
    assert(cui_bridge_icon_create_v1(a, &l, &h[3]) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(a, h[3], 3, data, mib) == CUI_BRIDGE_CAPACITY);
    assert(info(a, h[3]).source_kind == 0);
    cui_bridge_destroy(a);
    a = create();
    l = limits();
    l.max_svg_bytes = 1;
    l.max_cache_bytes = 1;
    assert(cui_bridge_icon_create_v1(a, &l, &h[0]) == CUI_BRIDGE_OK);
    char path[4096];
    memset(path, 'p', sizeof path);
    assert(cui_bridge_icon_set_source_v1(a, h[0], 2, path, sizeof path) == CUI_BRIDGE_OK);
    assert(a->icons->copied_bytes == 8194 && a->icons->svg_quota == 1);
    l.max_svg_bytes = 0;
    assert(cui_bridge_icon_set_limits_v1(a, h[0], &l) == CUI_BRIDGE_OK);
    assert(a->icons->copied_bytes == 8194);
    cui_bridge_destroy(a);
    // Actual payload fills the8MiB admission boundary exactly. Even a smaller
    // replacement needs temporary headroom while the old map entry stays live.
    a = create();
    l = limits();
    l.max_svg_bytes = mib;
    l.max_cache_bytes = 1;
    assert(cui_bridge_icon_create_v1(a, &l, &h[0]) == CUI_BRIDGE_OK);
    for (char key = 'a'; key <= 'f'; key++)
        assert(cui_bridge_icon_asset_put_v1(a, h[0], &key, 1, data, mib) == CUI_BRIDGE_OK);
    uint64_t remaining = 8 * (uint64_t)mib - a->icons->svg_quota - a->icons->copied_bytes;
    assert(remaining > 3 && remaining - 3 < mib);
    assert(cui_bridge_icon_asset_put_v1(a, h[0], "g", 1, data, (uint32_t)remaining - 3) ==
           CUI_BRIDGE_OK);
    assert(a->icons->svg_quota + a->icons->copied_bytes == 8 * (uint64_t)mib);
    uint64_t revision = info(a, h[0]).source_revision;
    assert(cui_bridge_icon_asset_put_v1(a, h[0], "g", 1, "x", 1) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_icon_set_source_v1(a, h[0], 0, NULL, 0) == CUI_BRIDGE_CAPACITY);
    assert(info(a, h[0]).source_revision == revision);
    assert(cui_bridge_icon_asset_remove_v1(a, h[0], "g", 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_asset_put_v1(a, h[0], "g", 1, "x", 1) == CUI_BRIDGE_OK);
    cui_bridge_destroy(a);
    free(data);
}
static void aliases(void) {
    cui_bridge_app *a = create();
    cui_bridge_icon_limits_v1 l = limits();
    cui_bridge_icon_handle_v1 h;
    cui_bridge_icon_limits_v1 saved_limits = l;
    assert(cui_bridge_icon_create_v1(a, &l, (cui_bridge_icon_handle_v1 *)&l.reserved) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&l, &saved_limits, sizeof l) && !a->icons);
    assert(cui_bridge_icon_create_v1(a, &l, &h) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(a, h, 3, square, sizeof square - 1) == CUI_BRIDGE_OK);
    uint32_t words[256], saved[256];
    memset(words, 0xa5, sizeof words);
    memcpy(saved, words, sizeof words);
    char *bytes = (char *)words;
    uint32_t separate = 123;
    assert(cui_bridge_icon_get_source_v1(a, h, &words[0], NULL, 0, &words[0]) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_get_source_v1(a, h, &separate, bytes, 2, &words[0]) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(words, saved, sizeof words) && separate == 123);
    assert(cui_bridge_icon_catalog_entry_v1(0, &words[0], NULL, 0, &words[1], NULL, 0, &words[1]) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_catalog_entry_v1(0, &words[0], bytes + 64, 40, &words[1], bytes + 72, 40,
                                            &words[2]) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_catalog_entry_v1(0, &words[0], bytes + 4, 1, &words[1], NULL, 0,
                                            &words[2]) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(words, saved, sizeof words));
    memcpy(bytes, "check", 5);
    memcpy(saved, words, sizeof words);
    assert(cui_bridge_icon_query_v1(bytes, 5, &words[0]) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_asset_copy_v1(bytes, 5, bytes + 1, 2, &separate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_asset_copy_v1("check", 5, bytes + 4, 1, &words[1]) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(words, saved, sizeof words) && separate == 123);
    float t[] = {1, 0, 0, 1, 0, 0}, before[6];
    memcpy(before, t, sizeof t);
    assert(cui_bridge_icon_prepare_v1(a, h, 16, 16, t, (uint32_t *)t) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(t, before, sizeof t));
    assert(prepare(a, h) == CUI_ICON_OK);
    cui_bridge_icon_bitmap_v1 b = {.struct_size = sizeof b, .schema_version = 1}, copy = b;
    assert(cui_bridge_icon_bitmap_copy_v1(a, h, &b, NULL, 0, &b.width) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_bitmap_copy_v1(a, h, &b, (unsigned char *)&b, 4, &separate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&b, &copy, sizeof b) && separate == 123);
    volatile uintptr_t high = UINTPTR_MAX - 1;
    char *wrapped = (char *)high;
    assert(cui_bridge_icon_asset_copy_v1("check", 5, wrapped, 4, &separate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_get_source_v1(a, h, &words[0], wrapped, 4, &separate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_catalog_entry_v1(0, &words[0], wrapped, 4, &words[1], NULL, 0,
                                            &words[2]) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_icon_bitmap_copy_v1(a, h, &b, (unsigned char *)wrapped, 4, &separate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&b, &copy, sizeof b) && !memcmp(words, saved, sizeof words) && separate == 123);
    cui_bridge_destroy(a);
}
int main(void) {
    retained_source_budget();
    aliases();
    public_records();
    resource_lifetime();
    maps_and_failures();
    quotas_and_lazy();
    puts("PASS owned Icon resource records/copies/maps/limits/lifecycle; presentation pending");
    return 0;
}
