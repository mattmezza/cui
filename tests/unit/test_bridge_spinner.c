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
}
