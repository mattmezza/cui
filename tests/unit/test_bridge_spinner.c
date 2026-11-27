/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <stdlib.h>

static unsigned allocations_until_failure;
static void *test_bridge_calloc(size_t count, size_t size) {
    if (allocations_until_failure && !--allocations_until_failure)
        return NULL;
    return calloc(count, size);
}

#define calloc test_bridge_calloc
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"
#undef calloc

#include "../fixtures/bridge_step_software.h"

static cui_bridge_node create(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                              const char *key) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    return node;
}

static cui_bridge_spinner_config_v1 config_default(void) {
    return (cui_bridge_spinner_config_v1){
        .struct_size = sizeof(cui_bridge_spinner_config_v1),
        .schema_version = CUI_BRIDGE_SPINNER_V1,
        .size = CUI_BRIDGE_SPINNER_MEDIUM,
    };
}

static cui_bridge_spinner_presentation_v1 sample(cui_bridge_app *app, cui_bridge_node node,
                                                 cui_bridge_status expected) {
    cui_bridge_spinner_presentation_v1 result = {
        .struct_size = sizeof result, .schema_version = 1, .frame_sequence = 123, .phase = -1};
    assert(cui_bridge_spinner_get_presentation_v1(app, node, &result) == expected);
    if (expected != CUI_BRIDGE_OK)
        assert(result.frame_sequence == 123 && result.phase == -1);
    return result;
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node, float size) {
    cui_bridge_rect rect;
    assert(cui_bridge_node_bounds(app, node, &rect) == CUI_BRIDGE_OK);
    assert(fabsf(rect.width - size) < .001f && fabsf(rect.height - size) < .001f);
    return rect;
}

typedef struct {
    unsigned painted, magenta;
    uint64_t fingerprint;
} pixel_stats;

static pixel_stats inspect_pixels(cui_bridge_app *app, cui_bridge_rect rect) {
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(software, &width, &height);
    cui_color background = app->theme.background;
    pixel_stats stats = {0};
    for (unsigned y = (unsigned)rect.y; y < (unsigned)(rect.y + rect.height); y++)
        for (unsigned x = (unsigned)rect.x; x < (unsigned)(rect.x + rect.width); x++) {
            assert(x < width && y < height);
            const unsigned char *p = pixels + ((size_t)y * width + x) * 4;
            unsigned different =
                p[0] != background.r || p[1] != background.g || p[2] != background.b;
            stats.painted += different;
            stats.magenta += p[0] == 255 && p[1] == 0 && p[2] == 255;
            stats.fingerprint = stats.fingerprint * 31 + different;
        }
    return stats;
}

static const char blade_svg[] = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
                                "<path d='M12 2 L20 14 L12 20 L6 13 L11 13 Z'/></svg>";
static const char block_svg[] = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
                                "<path d='M4 4 H20 V20 H4 Z'/></svg>";

static cui_bridge_icon_handle_v1 artwork(cui_bridge_app *app, const char *svg) {
    cui_bridge_icon_limits_v1 limits = {
        .struct_size = sizeof limits, .schema_version = 1, .max_versions = 4};
    cui_bridge_icon_handle_v1 handle;
    assert(cui_bridge_icon_create_v1(app, &limits, &handle) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, handle, CUI_BRIDGE_ICON_DATA_V1, svg,
                                         (uint32_t)strlen(svg)) == CUI_BRIDGE_OK);
    return handle;
}

static cui_bridge_icon_info_v1 icon_info(cui_bridge_app *app, cui_bridge_icon_handle_v1 handle) {
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, handle, &info) == CUI_BRIDGE_OK);
    return info;
}

static void test_shared_artwork(cui_bridge_options options) {
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK && cui_bridge_spinner_icon_resource_versions() == 1);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node badge_icon = create(app, root, CUI_BRIDGE_ICON, "identity");
    cui_bridge_node first = create(app, root, CUI_BRIDGE_SPINNER, "sync");
    cui_bridge_node second = create(app, root, CUI_BRIDGE_SPINNER, "upload");
    cui_bridge_icon_handle_v1 a = artwork(app, blade_svg);
    cui_bridge_icon_handle_v1 b = artwork(app, block_svg);
    const cui_bridge_icon_handle_v1 null = {0};

    cui_bridge_icon_handle_v1 readback = {123, 456};
    assert(cui_bridge_spinner_get_icon_resource_v1(app, first, &readback) == CUI_BRIDGE_EMPTY);
    assert(readback.slot == 123 && readback.generation == 456);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, a) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, root, a) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_spinner_get_icon_resource_v1(app, first, NULL) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    const cui_bridge_icon_options_v1 style = {.struct_size = sizeof style,
                                              .schema_version = 1,
                                              .size = CUI_ICON_CUSTOM,
                                              .custom_size = 18};
    assert(cui_bridge_icon_configure_node_v1(app, badge_icon, a, &style, NULL, 0) == CUI_BRIDGE_OK);
    cui_bridge_spinner_config_v1 c = config_default();
    c.size = CUI_BRIDGE_SPINNER_CUSTOM;
    c.pixels = 32;
    c.duration = 1;
    c.easing = CUI_BRIDGE_SPINNER_LINEAR;
    c.flags = CUI_BRIDGE_SPINNER_CUSTOM_COLOR;
    c.rgba = UINT32_C(0xFF00FFFF);
    strcpy(c.icon, "arrow-right");
    assert(cui_bridge_spinner_configure_v1(app, first, &c) == CUI_BRIDGE_OK);
    c.pixels = 24;
    assert(cui_bridge_spinner_configure_v1(app, second, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .125f) == CUI_BRIDGE_OK);
    cui_bridge_rect rect = bounds(app, first, 32);
    uint64_t catalog = inspect_pixels(app, rect).fingerprint;

    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, a) == CUI_BRIDGE_OK);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, second, a) == CUI_BRIDGE_OK);
    assert(cui_bridge_spinner_get_icon_resource_v1(app, first, &readback) == CUI_BRIDGE_OK);
    assert(readback.slot == a.slot && readback.generation == a.generation);
    (void)sample(app, first, CUI_BRIDGE_EMPTY);
    assert(icon_info(app, a).reference_count == 3);
    assert(cui_bridge_icon_destroy_v1(app, a) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    pixel_stats start = inspect_pixels(app, rect);
    assert(start.magenta && start.fingerprint != catalog);
    uint64_t identity = inspect_pixels(app, bounds(app, badge_icon, 18)).fingerprint;
    assert(identity);

    /* A single resource supplies three distinct painted variants per frame. */
    for (unsigned i = 0; i < 90; i++) {
        assert(cui_bridge_step(app, .013f) == CUI_BRIDGE_OK);
        assert(inspect_pixels(app, rect).magenta);
        assert(inspect_pixels(app, bounds(app, second, 24)).magenta);
        assert(inspect_pixels(app, bounds(app, badge_icon, 18)).fingerprint == identity);
        cui_bridge_icon_info_v1 info = icon_info(app, a);
        assert(info.cache_versions >= 3 && info.cache_versions <= 4 && info.parse_count == 1);
    }
    assert(inspect_pixels(app, rect).fingerprint != start.fingerprint);
    cui_bridge_spinner_presentation_v1 moving = sample(app, first, CUI_BRIDGE_OK);
    assert(moving.phase > 0);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, a) == CUI_BRIDGE_OK);
    assert(sample(app, first, CUI_BRIDGE_OK).frame_sequence == moving.frame_sequence);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, (cui_bridge_icon_handle_v1){0, 1}) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_spinner_set_icon_resource_v1(
               app, first, (cui_bridge_icon_handle_v1){a.slot, a.generation + 1}) ==
           CUI_BRIDGE_STALE_HANDLE);
    assert(sample(app, first, CUI_BRIDGE_OK).phase == moving.phase);

    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, b) == CUI_BRIDGE_OK);
    assert(icon_info(app, a).reference_count == 2 && icon_info(app, b).reference_count == 1);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    assert(sample(app, first, CUI_BRIDGE_OK).phase == moving.phase);
    uint64_t block = inspect_pixels(app, rect).fingerprint;
    assert(cui_bridge_icon_set_source_v1(app, b, CUI_BRIDGE_ICON_DATA_V1, blade_svg,
                                         sizeof blade_svg - 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    assert(icon_info(app, b).parse_count == 2 && inspect_pixels(app, rect).fingerprint != block);

    const char missing[] = "app/missing.svg";
    assert(cui_bridge_icon_set_source_v1(app, b, CUI_BRIDGE_ICON_PATH_V1, missing,
                                         sizeof missing - 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    (void)sample(app, first, CUI_BRIDGE_EMPTY);
    (void)sample(app, second, CUI_BRIDGE_OK);
    assert(icon_info(app, b).icon_error == CUI_ICON_NOT_FOUND);
    assert(!inspect_pixels(app, rect).painted);
    assert(cui_bridge_spinner_set_icon_resource_v1(app, first, null) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, b) == CUI_BRIDGE_OK);
    assert(cui_bridge_spinner_get_icon_resource_v1(app, first, &readback) == CUI_BRIDGE_EMPTY);
    assert(readback.slot == a.slot);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    assert(sample(app, first, CUI_BRIDGE_OK).phase == 0);

    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    assert(icon_info(app, a).reference_count == 1);
    assert(cui_bridge_node_remove(app, badge_icon) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, a) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}

int main(void) {
    cui_bridge_options options = {
        .struct_size = sizeof options,
        .abi_version = CUI_BRIDGE_ABI_VERSION,
        .backend = CUI_BRIDGE_BACKEND_SDL,
        .width = 320,
        .height = 240,
        .max_nodes = 8,
    };
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_spinner_versions() == 1 && cui_bridge_spinner_config_size_v1() == 100 &&
           cui_bridge_spinner_presentation_size_v1() == 40);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node legacy = create(app, root, CUI_BRIDGE_SPINNER, "legacy");
    cui_bridge_node icon = create(app, root, CUI_BRIDGE_SPINNER, "icon");
    cui_bridge_node panel = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "panel");
    cui_bridge_node conditional = create(app, panel, CUI_BRIDGE_SPINNER, "conditional");

    cui_bridge_spinner_config_v1 c = config_default();
    assert(cui_bridge_spinner_get_config_v1(app, legacy, &c) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_spinner_configure_v1(app, root, &c) == CUI_BRIDGE_INVALID_STATE);
    for (unsigned i = 1; i <= 2; i++) {
        allocations_until_failure = i;
        assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_OUT_OF_MEMORY);
        assert(!app->spinners);
    }
    assert(cui_bridge_set_visual_size(app, legacy, 22, 99) == CUI_BRIDGE_OK);
    c.size = CUI_BRIDGE_SPINNER_LARGE;
    c.rem_size = 20;
    c.flags = CUI_BRIDGE_SPINNER_CUSTOM_COLOR;
    c.rgba = UINT32_C(0xFF00FFFF);
    strcpy(c.icon, "arrow-right");
    c.icon[sizeof c.icon - 1] = 'x';
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, icon, 8, 8) == CUI_BRIDGE_INVALID_STATE);
    memset(c.icon, 'x', sizeof c.icon);
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    c.icon[0] = (char)0xff;
    c.icon[1] = 0;
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_INVALID_UTF8);
    strcpy(c.icon, "cui-no-such-asset");
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_spinner_get_config_v1(app, icon, &c) == CUI_BRIDGE_OK);
    assert(!strcmp(c.icon, "arrow-right") && !c.icon[sizeof c.icon - 1]);
    c.rem_size = 3000; /* A valid rem with an out-of-range resolved size. */
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    c = config_default();
    assert(cui_bridge_spinner_configure_v1(app, conditional, &c) == CUI_BRIDGE_OK);
    (void)sample(app, icon, CUI_BRIDGE_EMPTY);

    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    cui_bridge_rect square = bounds(app, icon, 30);
    pixel_stats first_pixels = inspect_pixels(app, square);
    assert(first_pixels.painted && first_pixels.magenta);
    (void)bounds(app, legacy, 22);
    (void)sample(app, legacy, CUI_BRIDGE_EMPTY);
    (void)sample(app, conditional, CUI_BRIDGE_EMPTY);
    cui_bridge_spinner_presentation_v1 first = sample(app, icon, CUI_BRIDGE_OK);
    assert(first.frame_sequence && first.core_time == .25 && first.phase == 0 && !first.reserved);
    assert(fabs(first.next_wake_seconds - (.25 + 1.0 / 60)) < 1e-9);
    size_t count;
    const cui_semantic *semantics = cui_semantics(app->ui, &count);
    bool status_found = false;
    for (size_t i = 0; i < count; i++) {
        assert(semantics[i].id != app->nodes[icon.slot].id);
        if (semantics[i].id == app->nodes[legacy.slot].id)
            status_found = semantics[i].role == CUI_ROLE_STATUS;
    }
    assert(status_found);

    assert(cui_bridge_step(app, .2f) == CUI_BRIDGE_OK);
    assert(fabsf(sample(app, icon, CUI_BRIDGE_OK).phase - .125f) < 1e-6f);
    assert(cui_bridge_spinner_get_config_v1(app, icon, &c) == CUI_BRIDGE_OK);
    c.easing = CUI_BRIDGE_SPINNER_LINEAR;
    c.duration = .8f; /* Same effective period as zero. */
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_OK);
    (void)sample(app, icon, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    assert(fabsf(sample(app, icon, CUI_BRIDGE_OK).phase - .25f) < 1e-6f);
    assert(inspect_pixels(app, square).fingerprint != first_pixels.fingerprint);
    c.duration = .5f;
    c.size = CUI_BRIDGE_SPINNER_CUSTOM;
    c.pixels = 26;
    c.rem_size = 0;
    c.rgba = 0; /* Transparent black is still an authored color. */
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(sample(app, icon, CUI_BRIDGE_OK).phase == 0);
    assert(!inspect_pixels(app, bounds(app, icon, 26)).painted);

    render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(app, .125f) == CUI_BRIDGE_CORE_ERROR);
    (void)sample(app, icon, CUI_BRIDGE_EMPTY);
    render_status = CUI_BRIDGE_OK;
    assert(cui_bridge_step(app, .125f) == CUI_BRIDGE_OK);
    assert(sample(app, icon, CUI_BRIDGE_OK).phase == .5f);
    c.flags |= CUI_BRIDGE_SPINNER_REDUCED_MOTION;
    assert(cui_bridge_spinner_configure_v1(app, icon, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .125f) == CUI_BRIDGE_OK);
    cui_bridge_spinner_presentation_v1 still = sample(app, icon, CUI_BRIDGE_OK);
    assert(still.phase == 0 && !still.next_wake_seconds);

    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    (void)bounds(app, conditional, 16);
    assert(sample(app, conditional, CUI_BRIDGE_OK).phase == 0);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(sample(app, conditional, CUI_BRIDGE_OK).phase > 0);
    assert(cui_bridge_set_open(app, panel, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    (void)sample(app, conditional, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(sample(app, conditional, CUI_BRIDGE_OK).phase == 0);
    assert(cui_bridge_node_remove(app, icon) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = create(app, root, CUI_BRIDGE_SPINNER, "replacement");
    assert(replacement.slot == icon.slot && replacement.generation != icon.generation);
    (void)sample(app, icon, CUI_BRIDGE_STALE_HANDLE);
    (void)sample(app, replacement, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    (void)bounds(app, replacement, 16);
    cui_bridge_event event;
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    test_shared_artwork(options);
}
