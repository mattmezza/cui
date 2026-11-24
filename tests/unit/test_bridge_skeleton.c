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

/* Run the actual bridge step and software backend using a simulated native host. */
static cui_software *software;
static cui_bridge_status render_status;

cui_bridge_status cui_bridge_native_create(cui_bridge_app *app, const char *title, const char *font,
                                           cui_config *config, bool hidden) {
    (void)title;
    (void)font;
    (void)config;
    (void)hidden;
    software = cui_software_create((unsigned)app->width, (unsigned)app->height, NULL);
    return software ? CUI_BRIDGE_OK : CUI_BRIDGE_OUT_OF_MEMORY;
}

void cui_bridge_native_destroy(cui_bridge_app *app) {
    (void)app;
    cui_software_destroy(software);
    software = NULL;
}

cui_bridge_status cui_bridge_native_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                        void *user) {
    (void)app;
    (void)callback;
    (void)user;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_poll(cui_bridge_app *app) {
    (void)app;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_native_render(cui_bridge_app *app, const cui_draw_list *list) {
    if (render_status != CUI_BRIDGE_OK)
        return render_status;
    assert(cui_software_resize(software, (unsigned)app->width, (unsigned)app->height));
    return cui_software_render(software, list, (cui_color){0, 0, 0, 255}) ? CUI_BRIDGE_OK
                                                                          : CUI_BRIDGE_CORE_ERROR;
}

cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *app,
                                                       const char *const *paths, uint32_t count) {
    (void)app;
    (void)paths;
    (void)count;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *app, cui_str text) {
    (void)app;
    (void)text;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *app, char *text,
                                                  uint32_t capacity, uint32_t *length) {
    (void)app;
    (void)text;
    (void)capacity;
    (void)length;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *app, uint32_t visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *app, uint32_t *visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_node create(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                              const char *key) {
    cui_bridge_node result;
    assert(cui_bridge_node_create(app, parent, kind, key, &result) == CUI_BRIDGE_OK);
    return result;
}

static cui_bridge_skeleton_config_v1 defaults(void) {
    return (cui_bridge_skeleton_config_v1){
        .struct_size = sizeof(cui_bridge_skeleton_config_v1),
        .schema_version = 1,
        .width_mode = CUI_BRIDGE_SKELETON_FRACTION,
        .width = 1,
        .height = 16,
    };
}

static cui_bridge_skeleton_presentation_v1 read_state(cui_bridge_app *app, cui_bridge_node node,
                                                      cui_bridge_status expected) {
    cui_bridge_skeleton_presentation_v1 result = {
        .struct_size = sizeof result, .schema_version = 1, .frame_sequence = 987};
    assert(cui_bridge_skeleton_get_presentation_v1(app, node, &result) == expected);
    if (expected != CUI_BRIDGE_OK)
        assert(result.frame_sequence == 987 && result.opacity == 0);
    return result;
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node, float width,
                              float height) {
    cui_bridge_rect rect;
    assert(cui_bridge_node_bounds(app, node, &rect) == CUI_BRIDGE_OK);
    assert(fabsf(rect.width - width) < .001f && fabsf(rect.height - height) < .001f);
    return rect;
}

static cui_color pixel(float x, float y) {
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(software, &width, &height);
    assert(pixels && x >= 0 && y >= 0 && x < width && y < height);
    const unsigned char *rgba = pixels + ((size_t)y * width + (unsigned)x) * 4;
    return (cui_color){rgba[0], rgba[1], rgba[2], rgba[3]};
}

static void check_color(cui_color actual, cui_color expected) {
    assert(actual.r == expected.r && actual.g == expected.g && actual.b == expected.b &&
           actual.a == expected.a);
}

static void theme(cui_bridge_app *app, const char *wanted) {
    for (uint32_t i = 0; i < cui_bridge_theme_preset_count(); i++) {
        char name[128];
        uint32_t length;
        assert(cui_bridge_theme_preset_name(i, name, sizeof name, &length) == CUI_BRIDGE_OK);
        if (!strcmp(wanted, name)) {
            assert(cui_bridge_set_theme_preset(app, i) == CUI_BRIDGE_OK);
            return;
        }
    }
    assert(!"preset does not exist");
}

int main(void) {
    cui_bridge_options options = {
        .struct_size = sizeof options,
        .abi_version = CUI_BRIDGE_ABI_VERSION,
        .backend = CUI_BRIDGE_BACKEND_SDL,
        .width = 320,
        .height = 180,
        .max_nodes = 8,
    };
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_skeleton_versions() == 1 && cui_bridge_skeleton_config_size_v1() == 40 &&
           cui_bridge_skeleton_presentation_size_v1() == 40);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node legacy = create(app, root, CUI_BRIDGE_SKELETON, "legacy");
    cui_bridge_node circle = create(app, root, CUI_BRIDGE_SKELETON, "circle");
    cui_bridge_node bar = create(app, root, CUI_BRIDGE_SKELETON, "bar");

    cui_bridge_skeleton_config_v1 c = defaults();
    assert(cui_bridge_skeleton_configure_v1(app, root, &c) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_skeleton_get_config_v1(app, legacy, &c) == CUI_BRIDGE_EMPTY);
    allocations_until_failure = 2;
    assert(cui_bridge_skeleton_configure_v1(app, circle, &c) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(!app->skeletons);
    assert(cui_bridge_set_visual_size(app, legacy, 72, 12) == CUI_BRIDGE_OK);
    c.width_mode = CUI_BRIDGE_SKELETON_FIXED;
    c.width = 40;
    c.height = 40;
    c.radius = 20;
    assert(cui_bridge_skeleton_configure_v1(app, circle, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, circle, 8, 8) == CUI_BRIDGE_INVALID_STATE);
    c = defaults();
    c.width = .5f;
    c.flags = CUI_BRIDGE_SKELETON_SECONDARY | CUI_BRIDGE_SKELETON_REDUCED_MOTION;
    assert(cui_bridge_skeleton_configure_v1(app, bar, &c) == CUI_BRIDGE_OK);
    c.max_width = 10;
    assert(cui_bridge_skeleton_configure_v1(app, bar, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    c = defaults();
    c.width = NAN;
    assert(cui_bridge_skeleton_configure_v1(app, bar, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_skeleton_get_config_v1(app, bar, &c) == CUI_BRIDGE_OK);
    assert(c.width == .5f && c.max_width == 0 && c.flags == 5);
    c.schema_version = 2;
    assert(cui_bridge_skeleton_get_config_v1(app, bar, &c) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(c.schema_version == 2);
    (void)read_state(app, circle, CUI_BRIDGE_EMPTY);

    theme(app, "Aurora Light");
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    (void)bounds(app, legacy, 72, 12);
    cui_bridge_rect round = bounds(app, circle, 40, 40);
    cui_bridge_rect line = bounds(app, bar, 144, 16);
    cui_color background = pixel(0, 0);
    check_color(pixel(round.x + 20, round.y + 20), (cui_color){241, 245, 249, 255});
    check_color(pixel(round.x, round.y), background);
    cui_color secondary = pixel(line.x + 20, line.y + 8);
    assert(abs((int)secondary.r - ((int)background.r + 241) / 2) <= 1);
    assert(abs((int)secondary.g - ((int)background.g + 245) / 2) <= 1);
    assert(abs((int)secondary.b - ((int)background.b + 249) / 2) <= 1);
    cui_bridge_skeleton_presentation_v1 first = read_state(app, circle, CUI_BRIDGE_OK);
    assert(first.frame_sequence && first.core_time == .25 && first.opacity == 1 &&
           first.phase == 0);
    assert(fabs(first.next_wake_seconds - (.25 + 1.0 / 60)) < 1e-9);
    cui_bridge_skeleton_presentation_v1 static_bar = read_state(app, bar, CUI_BRIDGE_OK);
    assert(static_bar.phase == 0 && static_bar.opacity == 1 && static_bar.next_wake_seconds == 0);
    size_t count;
    const cui_semantic *semantics = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].id != app->nodes[circle.slot].id &&
               semantics[i].id != app->nodes[bar.slot].id);
    (void)read_state(app, legacy, CUI_BRIDGE_EMPTY);

    /* A new viewport and restyle use existing Core layout and preserve pulse time. */
    app->width = 192;
    c = defaults();
    assert(cui_bridge_skeleton_get_config_v1(app, circle, &c) == CUI_BRIDGE_OK);
    c.flags = CUI_BRIDGE_SKELETON_CUSTOM_COLOR | CUI_BRIDGE_SKELETON_SECONDARY;
    c.background_rgba = UINT32_C(0x0022EEFF);
    c.duration = 2; /* Same effective period as zero. */
    assert(cui_bridge_skeleton_configure_v1(app, circle, &c) == CUI_BRIDGE_OK);
    (void)read_state(app, circle, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_step(app, .5f) == CUI_BRIDGE_OK);
    (void)bounds(app, bar, 80, 16);
    cui_bridge_skeleton_presentation_v1 second = read_state(app, circle, CUI_BRIDGE_OK);
    assert(second.frame_sequence > first.frame_sequence && second.phase == .5f &&
           second.opacity == .75f);
    cui_color tinted = pixel(round.x + 20, round.y + 20);
    assert(abs((int)tinted.r - (int)background.r / 4) <= 1);
    assert(abs((int)tinted.g - ((int)background.g + 3 * 34) / 4) <= 1);
    assert(abs((int)tinted.b - ((int)background.b + 3 * 238) / 4) <= 1);
    c.background_rgba = 0; /* Authored transparent black remains a custom color. */
    c.duration = 1;
    assert(cui_bridge_skeleton_configure_v1(app, circle, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(read_state(app, circle, CUI_BRIDGE_OK).phase == 0);
    check_color(pixel(round.x + 20, round.y + 20), background);

    render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_CORE_ERROR);
    (void)read_state(app, circle, CUI_BRIDGE_EMPTY);
    render_status = CUI_BRIDGE_OK;
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(read_state(app, circle, CUI_BRIDGE_OK).opacity == .5f);

    c = defaults();
    c.width_mode = CUI_BRIDGE_SKELETON_GROW;
    c.width = 0;
    c.max_width = 64;
    assert(cui_bridge_skeleton_configure_v1(app, bar, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .125f) == CUI_BRIDGE_OK);
    (void)bounds(app, bar, 64, 16);
    assert(read_state(app, bar, CUI_BRIDGE_OK).phase == 0);
    assert(read_state(app, circle, CUI_BRIDGE_OK).phase > 0);

    cui_bridge_node collapsible = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "collapsible");
    cui_bridge_node nested = create(app, collapsible, CUI_BRIDGE_SKELETON, "nested");
    c = defaults();
    c.width_mode = CUI_BRIDGE_SKELETON_GROW;
    c.width = 0;
    c.max_width = 64;
    assert(cui_bridge_skeleton_configure_v1(app, nested, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, collapsible, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(read_state(app, nested, CUI_BRIDGE_OK).phase == 0);
    assert(cui_bridge_set_open(app, collapsible, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    (void)read_state(app, nested, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_open(app, collapsible, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .25f) == CUI_BRIDGE_OK);
    assert(read_state(app, nested, CUI_BRIDGE_OK).phase == 0);
    assert(cui_bridge_node_remove(app, collapsible) == CUI_BRIDGE_OK);
    (void)read_state(app, nested, CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_node_remove(app, circle) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = create(app, root, CUI_BRIDGE_SKELETON, "replacement");
    assert(replacement.slot == circle.slot && replacement.generation != circle.generation);
    (void)read_state(app, replacement, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    (void)bounds(app, replacement, 120, 16);
    (void)read_state(app, circle, CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_event_ex event;
    uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &event, NULL, 0, &required) == CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
