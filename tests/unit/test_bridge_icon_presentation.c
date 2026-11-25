/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <stdlib.h>

static unsigned allocations_until_failure;
static void *test_calloc(size_t count, size_t size) {
    if (allocations_until_failure && !--allocations_until_failure)
        return NULL;
    return calloc(count, size);
}

#define calloc test_calloc
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"
#undef calloc
#include "../fixtures/bridge_step_software.h"

static const char svg[] = "<svg width='16' height='16'><rect x='1' y='2' "
                          "width='5' height='9'/></svg>";

static cui_bridge_icon_options_v1 defaults(void) {
    return (cui_bridge_icon_options_v1){.struct_size = sizeof(cui_bridge_icon_options_v1),
                                        .schema_version = 1};
}

static cui_bridge_app *make_app(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = 1,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 112,
                                  .height = 88};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    return app;
}

static cui_bridge_node node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                            const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(app, parent, kind, key, &n) == CUI_BRIDGE_OK);
    return n;
}

static cui_bridge_icon_handle_v1 resource(cui_bridge_app *app) {
    cui_bridge_icon_limits_v1 limits = {.struct_size = sizeof limits, .schema_version = 1};
    cui_bridge_icon_handle_v1 h;
    assert(cui_bridge_icon_create_v1(app, &limits, &h) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, h, CUI_BRIDGE_ICON_DATA_V1, svg, sizeof svg - 1) ==
           CUI_BRIDGE_OK);
    return h;
}

static cui_bridge_icon_result_v1 result(cui_bridge_app *app, cui_bridge_node n,
                                        cui_bridge_status status) {
    cui_bridge_icon_result_v1 value = {
        .struct_size = sizeof value, .schema_version = 1, .frame_sequence = UINT64_MAX};
    assert(cui_bridge_icon_get_node_result_v1(app, n, &value) == status);
    if (status == CUI_BRIDGE_OK) {
        assert(value.node.slot == n.slot && value.node.generation == n.generation &&
               !value.reserved && value.frame_sequence == app->last_core_frame);
    } else
        assert(value.frame_sequence == UINT64_MAX);
    return value;
}

static uint32_t references(cui_bridge_app *app, cui_bridge_icon_handle_v1 h) {
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, h, &info) == CUI_BRIDGE_OK);
    return info.reference_count;
}

static void pixel_oracle(void) {
    cui_bridge_app *app = make_app();
    cui_bridge_node root = node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node icon = node(app, root, CUI_BRIDGE_ICON, "artwork");
    cui_bridge_icon_handle_v1 handle = resource(app);
    cui_context *oracle = cui_create((cui_config){.width = 112, .height = 88});
    cui_software *reference = cui_software_create(112, 88, NULL);
    cui_icon_state native = {0};
    assert(oracle && reference && cui_icon_set_data(&native, cui_string(svg)));
    for (unsigned mode = 0; mode < 5; mode++) {
        cui_bridge_icon_options_v1 c = defaults();
        char name[] = "Workspace symbol";
        c.flags = CUI_BRIDGE_ICON_WIDTH_V1 | CUI_BRIDGE_ICON_HEIGHT_V1 |
                  CUI_BRIDGE_ICON_OPACITY_V1 | CUI_BRIDGE_ICON_TRANSFORM_V1 |
                  CUI_BRIDGE_ICON_STYLE_V1 | CUI_BRIDGE_ICON_CONSTRAINTS_V1 |
                  CUI_BRIDGE_ICON_INHERITED_COLOR_V1;
        c.width = 19;
        c.height = 14;
        c.inherited_text_size = 23;
        c.opacity = .75f;
        c.inherited_color_rgba = UINT32_C(0xe75530bb);
        c.transform[1] = 1;
        c.transform[2] = -1;
        c.transform[4] = 3.25f;
        c.style = (cui_bridge_icon_style_v1){
            .struct_size = sizeof c.style,
            .schema_version = 1,
            .flags = CUI_BRIDGE_ICON_BOX_CORNERS_V1 | CUI_BRIDGE_ICON_BOX_BORDER_EDGES_V1 |
                     CUI_BRIDGE_ICON_BOX_PADDING_EDGES_V1 | CUI_BRIDGE_ICON_BOX_OPACITY_V1,
            .width = 100,
            .height = 100,
            .padding = 13,
            .radius = 10,
            .background_rgba = UINT32_C(0x4488eeaa),
            .border_rgba = UINT32_C(0xc0d0f0ff),
            .border_width = 4,
            .corners = {1, 2, 3, 4},
            .border_edges = {1, 2, 1, 2},
            .padding_edges = {2, 3, 1, 2},
            .opacity = .8f,
        };
        c.constraints = (cui_bridge_icon_constraints_v1){
            .struct_size = sizeof c.constraints,
            .schema_version = 1,
            .flags = CUI_BRIDGE_ICON_MAX_WIDTH_V1 | CUI_BRIDGE_ICON_MIN_HEIGHT_V1,
            .max_width = 12,
            .min_height = 21,
        };
        cui_box style = {
            .width = 100,
            .height = 100,
            .padding = 13,
            .radius = 10,
            .background = {0x44, 0x88, 0xee, 0xaa},
            .border = {0xc0, 0xd0, 0xf0, 0xff},
            .border_width = 4,
            .corner_radii = {1, 2, 3, 4},
            .border_edges = {1, 2, 1, 2},
            .padding_edges = {2, 3, 1, 2},
            .opacity = .8f,
            .custom_corners = true,
            .custom_border_edges = true,
            .custom_padding = true,
            .custom_opacity = true,
        };
        cui_icon_constraints limits = {
            .max_width = 12, .min_height = 21, .custom_max_width = true, .custom_min_height = true};
        cui_icon_options expected = {
            .width = 19,
            .height = 14,
            .custom_width = true,
            .custom_height = true,
            .inherited_text_size = 23,
            .color = {0},
            .inherited_color = {0xe7, 0x55, 0x30, 0xbb},
            .custom_inherited_color = true,
            .opacity = .75f,
            .custom_opacity = true,
            .transform = {0, 1, -1, 0, 3.25f, 0},
            .custom_transform = true,
            .style = &style,
            .constraints = &limits,
            .accessible_name = CUI_STR("Workspace symbol"),
        };
        if (mode == 1) {
            c.flags |= CUI_BRIDGE_ICON_COLOR_V1 | CUI_BRIDGE_ICON_DECORATIVE_V1;
            expected.custom_color = expected.decorative = true; /* transparent black */
        } else if (mode == 2) {
            c.constraints.max_width = 0;
            limits.max_width = 0; /* padding and border floor the explicit maximum */
        } else if (mode == 3) {
            c.size = CUI_ICON_LARGE;
            c.rem_size = 20;
            c.constraints.max_width = 30;
            expected.size = CUI_ICON_LARGE;
            expected.rem_size = 20;
            limits.max_width = 30;
        } else if (mode == 4) {
            c.flags |= CUI_BRIDGE_ICON_DEBUG_PLACEHOLDER_V1;
            expected.debug_placeholder = true;
            const char path[] = "assets/not-present.svg";
            assert(cui_bridge_icon_set_source_v1(app, handle, CUI_BRIDGE_ICON_PATH_V1, path,
                                                 sizeof path - 1) == CUI_BRIDGE_OK);
            assert(cui_icon_set_path(&native, cui_string(path), NULL, NULL));
        }
        assert(cui_bridge_icon_configure_node_v1(app, icon, handle, &c, name, sizeof name - 1) ==
               CUI_BRIDGE_OK);
        memset(name, 0, sizeof name);
        (void)result(app, icon, CUI_BRIDGE_EMPTY);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        assert(result(app, icon, CUI_BRIDGE_OK).icon_error ==
               (mode == 4 ? CUI_ICON_NOT_FOUND : CUI_ICON_OK));
        assert(cui_begin_frame(oracle, 112, 88, .016f));
        cui_begin_box(oracle, 1,
                      (cui_box){.width = -1, .height = -1, .background = app->theme.background});
        cui_begin_box(oracle, 2, (cui_box){.width = -1, .height = -1, .padding = 16});
        assert(cui_icon(oracle, 3, &app->theme, &native, expected) ==
               (mode == 4 ? CUI_ICON_NOT_FOUND : CUI_ICON_OK));
        cui_end_box(oracle);
        cui_end_box(oracle);
        cui_draw_list list = cui_end_frame(oracle);
        assert(cui_software_render(reference, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h, ow, oh;
        const unsigned char *current = cui_software_pixels(software, &w, &h);
        const unsigned char *exact = cui_software_pixels(reference, &ow, &oh);
        assert(w == ow && h == oh && !memcmp(current, exact, (size_t)w * h * 4));
        cui_bridge_rect measured;
        assert(cui_bridge_node_bounds(app, icon, &measured) == CUI_BRIDGE_OK);
        cui_rect direct = cui_bounds(oracle, 3);
        assert(measured.x == direct.x && measured.y == direct.y && measured.width == direct.w &&
               measured.height == direct.h);
        size_t count, expected_count;
        const cui_semantic *sem = cui_semantics(app->ui, &count);
        const cui_semantic *expected_sem = cui_semantics(oracle, &expected_count);
        assert(count == expected_count && count == (mode == 1 ? 0u : 1u));
        if (count)
            assert(sem[0].role == CUI_ROLE_IMAGE && sem[0].role == expected_sem[0].role &&
                   sem[0].name.length == expected_sem[0].name.length &&
                   !memcmp(sem[0].name.data, expected_sem[0].name.data, sem[0].name.length));
    }
    cui_icon_state_destroy(&native);
    cui_software_destroy(reference);
    cui_destroy(oracle);
    cui_bridge_destroy(app);
}

static void owned_lifecycle(void) {
    cui_bridge_app *app = make_app();
    cui_bridge_node root = node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node panel = node(app, root, CUI_BRIDGE_COLLAPSIBLE, "panel");
    cui_bridge_node icon = node(app, panel, CUI_BRIDGE_ICON, "icon");
    cui_bridge_icon_handle_v1 art = resource(app), second = resource(app);
    cui_bridge_icon_options_v1 config = defaults();
    assert(cui_bridge_set_text(app, icon, "check") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, icon, 15, 12) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    cui_bridge_icon_handle_v1 copied = {UINT32_MAX, UINT32_MAX};
    assert(cui_bridge_icon_get_node_config_v1(app, icon, &copied, &config) == CUI_BRIDGE_EMPTY);
    assert(copied.slot == UINT32_MAX);
    for (unsigned failure = 1; failure <= 2; failure++) {
        allocations_until_failure = failure;
        assert(cui_bridge_icon_configure_node_v1(app, icon, art, &config, NULL, 0) ==
               CUI_BRIDGE_OUT_OF_MEMORY);
        assert(!app->icons->presentations && references(app, art) == 0);
    }
    assert(cui_bridge_icon_configure_node_v1(app, icon, art, &config, "Profile", 7) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, art) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, icon, "legacy") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_visual_size(app, icon, 1, 1) == CUI_BRIDGE_INVALID_STATE);
    char name[16] = "";
    uint32_t needed = 0;
    assert(cui_bridge_icon_copy_node_name_v1(app, icon, name, 4, &needed) == CUI_BRIDGE_CAPACITY);
    assert(needed == 8 && name[0] == 0);
    assert(cui_bridge_icon_copy_node_name_v1(app, icon, name, sizeof name, &needed) ==
           CUI_BRIDGE_OK);
    assert(strcmp(name, "Profile") == 0 && needed == 8);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint64_t observed = result(app, icon, CUI_BRIDGE_OK).frame_sequence;
    cui_bridge_icon_options_v1 bad = defaults();
    bad.width = 12;
    assert(cui_bridge_icon_configure_node_v1(app, icon, second, &bad, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    const char invalid[] = {(char)0xff};
    assert(cui_bridge_icon_configure_node_v1(app, icon, second, &config, invalid, 1) ==
           CUI_BRIDGE_INVALID_UTF8);
    allocations_until_failure = 1;
    assert(cui_bridge_icon_configure_node_v1(app, icon, second, &config, "Other", 5) ==
           CUI_BRIDGE_OUT_OF_MEMORY);
    assert(result(app, icon, CUI_BRIDGE_OK).frame_sequence == observed);
    assert(references(app, art) == 1 && references(app, second) == 0);
    render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR);
    cui_bridge_icon_result_v1 rendered = result(app, icon, CUI_BRIDGE_OK);
    assert(rendered.icon_error == CUI_ICON_OK && rendered.frame_status == CUI_BRIDGE_CORE_ERROR);
    render_status = CUI_BRIDGE_OK;
    poll_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR);
    (void)result(app, icon, CUI_BRIDGE_EMPTY);
    poll_status = CUI_BRIDGE_OK;
    assert(cui_bridge_set_open(app, panel, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    (void)result(app, icon, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_configure_node_v1(app, icon, second, &config, "", 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_get_node_config_v1(app, icon, &copied, &config) == CUI_BRIDGE_OK);
    assert(copied.slot == second.slot && copied.generation == second.generation);
    assert(references(app, art) == 0 && references(app, second) == 1);
    assert(cui_bridge_icon_destroy_v1(app, art) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_reset_node_v1(app, icon) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect legacy;
    assert(cui_bridge_node_bounds(app, icon, &legacy) == CUI_BRIDGE_OK && legacy.width == 15 &&
           legacy.height == 12);
    assert(cui_bridge_icon_configure_node_v1(app, icon, second, &config, NULL, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, panel) == CUI_BRIDGE_OK);
    (void)result(app, icon, CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_icon_destroy_v1(app, second) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}

static void shared_resource(void) {
    cui_bridge_app *app = make_app();
    cui_bridge_node root = node(app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root");
    cui_bridge_node small = node(app, root, CUI_BRIDGE_ICON, "small");
    cui_bridge_node large = node(app, root, CUI_BRIDGE_ICON, "large");
    cui_bridge_icon_handle_v1 art = resource(app);
    cui_bridge_icon_options_v1 config = defaults();
    config.size = CUI_ICON_CUSTOM;
    config.custom_size = 16;
    assert(cui_bridge_icon_configure_node_v1(app, small, art, &config, NULL, 0) == CUI_BRIDGE_OK);
    config.custom_size = 32;
    assert(cui_bridge_icon_configure_node_v1(app, large, art, &config, NULL, 0) == CUI_BRIDGE_OK);
    assert(references(app, art) == 2);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(result(app, small, CUI_BRIDGE_OK).icon_error == CUI_ICON_OK);
    assert(result(app, large, CUI_BRIDGE_OK).icon_error == CUI_ICON_OK);
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, art, &info) == CUI_BRIDGE_OK);
    assert(info.raster_count == 2 && info.cache_versions == 2);
    cui_bridge_rect a, b;
    assert(cui_bridge_node_bounds(app, small, &a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, large, &b) == CUI_BRIDGE_OK);
    assert(a.width == 16 && a.height == 16 && b.width == 32 && b.height == 32);
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(software, &w, &h);
    size_t a_pixel = ((size_t)(a.y + 4) * w + (size_t)(a.x + 3)) * 4;
    size_t b_pixel = ((size_t)(b.y + 8) * w + (size_t)(b.x + 6)) * 4;
    assert(a_pixel + 3 < (size_t)w * h * 4 && b_pixel + 3 < (size_t)w * h * 4);
    assert(pixels[a_pixel] == app->theme.text.r && pixels[b_pixel] == app->theme.text.r);
    assert(cui_bridge_node_remove(app, small) == CUI_BRIDGE_OK);
    assert(references(app, art) == 1);
    assert(cui_bridge_icon_collect_v1(app, art) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(result(app, large, CUI_BRIDGE_OK).icon_error == CUI_ICON_OK);
    assert(cui_bridge_icon_get_info_v1(app, art, &info) == CUI_BRIDGE_OK);
    assert(info.raster_count == 2 && info.cache_versions == 1);
    cui_bridge_destroy(app);
}

int main(void) {
    assert(cui_bridge_icon_presentation_versions() == 1);
    assert(cui_bridge_icon_presentation_record_size_v1(CUI_BRIDGE_ICON_STYLE_RECORD_V1) == 128);
    assert(cui_bridge_icon_presentation_record_size_v1(CUI_BRIDGE_ICON_CONSTRAINTS_RECORD_V1) ==
           32);
    assert(cui_bridge_icon_presentation_record_size_v1(CUI_BRIDGE_ICON_OPTIONS_RECORD_V1) == 256);
    assert(cui_bridge_icon_presentation_record_size_v1(CUI_BRIDGE_ICON_RESULT_RECORD_V1) == 40);
    assert(cui_bridge_icon_presentation_record_size_v1(4) == 0);
    assert(cui_bridge_icon_record_size_v1(0) == 0);
    pixel_oracle();
    owned_lifecycle();
    shared_resource();
}
