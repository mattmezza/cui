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

/* The portable fixture has no font. Check text draw commands independently
 * while rendering all other commands through the real software painter. */
static cui_draw_list bridge_draw;
static bool draw_shapes(cui_software *target, const cui_draw_list *draw, cui_color clear) {
    cui_draw_command shapes[48];
    size_t count = 0;
    for (size_t i = 0; i < draw->count; ++i) {
        if (draw->commands[i].type != CUI_DRAW_TEXT) {
            assert(count < sizeof shapes / sizeof shapes[0]);
            shapes[count++] = draw->commands[i];
        }
    }
    cui_draw_list selection = *draw;
    selection.commands = shapes;
    selection.count = count;
    return cui_software_render(target, &selection, clear);
}

static bool draw_bridge(cui_software *target, const cui_draw_list *draw, cui_color clear) {
    bridge_draw = *draw;
    return draw_shapes(target, draw, clear);
}

#define CUI_BRIDGE_TEST_RENDER draw_bridge
#include "../fixtures/bridge_step_software.h"

static const char svg[] = "<svg width='16' height='16'><rect x='1' y='2' "
                          "width='5' height='9'/></svg>";

static cui_bridge_app *make_app(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 320,
                                  .height = 144};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    return app;
}

static cui_bridge_node create(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                              const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(app, parent, kind, key, &n) == CUI_BRIDGE_OK);
    return n;
}

static cui_bridge_badge_config_v1 defaults(void) {
    cui_bridge_badge_config_v1 cfg = {.struct_size = sizeof cfg, .schema_version = 1};
    assert(cui_bridge_badge_default_config_v1(&cfg) == CUI_BRIDGE_OK);
    assert(cfg.maximum == 99 && cfg.size == CUI_CONTROL_MEDIUM && cfg.variant == CUI_BADGE_COUNT);
    return cfg;
}

static cui_bridge_icon_handle_v1 art(cui_bridge_app *app) {
    cui_bridge_icon_limits_v1 limits = {.struct_size = sizeof limits, .schema_version = 1};
    cui_bridge_icon_handle_v1 resource;
    assert(cui_bridge_icon_create_v1(app, &limits, &resource) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, resource, CUI_BRIDGE_ICON_DATA_V1, svg,
                                         sizeof svg - 1) == CUI_BRIDGE_OK);
    return resource;
}

static cui_bridge_badge_result_v1 result(cui_bridge_app *app, cui_bridge_node badge,
                                         cui_bridge_status status) {
    cui_bridge_badge_result_v1 value = {
        .struct_size = sizeof value, .schema_version = 1, .frame_sequence = UINT64_MAX};
    assert(cui_bridge_badge_get_result_v1(app, badge, &value) == status);
    if (status == CUI_BRIDGE_OK) {
        assert(value.node.slot == badge.slot && value.node.generation == badge.generation);
        assert(value.frame_sequence == app->last_core_frame && value.submitted == 1);
    } else {
        assert(value.frame_sequence == UINT64_MAX);
    }
    return value;
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node n) {
    cui_bridge_rect b;
    assert(cui_bridge_node_bounds(app, n, &b) == CUI_BRIDGE_OK);
    return b;
}

static void click(cui_bridge_app *app, cui_bridge_node n, bool enabled) {
    cui_bridge_rect b = bounds(app, n);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, b.x + b.width / 2,
                                    b.y + b.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, b.x + b.width / 2,
                                    b.y + b.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event e;
    if (enabled) {
        assert(cui_bridge_poll_event(app, &e) == CUI_BRIDGE_OK);
        assert(e.type == CUI_BRIDGE_EVENT_CLICK && e.node.slot == n.slot &&
               e.node.generation == n.generation);
    }
    assert(cui_bridge_poll_event(app, &e) == CUI_BRIDGE_EMPTY);
}

static void inspect_semantics(cui_bridge_app *app, const char *name, bool visible) {
    size_t count;
    const cui_semantic *sem = cui_semantics(app->ui, &count);
    unsigned markers = 0, buttons = 0;
    for (size_t i = 0; i < count; ++i) {
        assert(sem[i].role != CUI_ROLE_IMAGE);
        if (sem[i].role == CUI_ROLE_STATUS) {
            markers++;
            assert(sem[i].name.length == strlen(name));
            assert(!memcmp(sem[i].name.data, name, sem[i].name.length));
        }
        buttons += sem[i].role == CUI_ROLE_BUTTON;
    }
    assert(buttons == 1 && markers == (unsigned)visible);
}

static void oracle_child(cui_context *ui, void *user) {
    const cui_theme *theme = user;
    (void)cui_button_ex(ui, 98, theme, cui_string("Inbox"),
                        (cui_button_options){.design_size = CUI_DESIGN_BASE});
}

static void compare_commands(cui_draw_list a, cui_draw_list b) {
    assert(a.count == b.count);
    for (size_t i = 0; i < a.count; ++i) {
        const cui_draw_command *x = &a.commands[i], *y = &b.commands[i];
        assert(x->type == y->type);
        if (x->type != CUI_DRAW_TEXT)
            continue;
        assert(x->text.length == y->text.length &&
               !memcmp(x->text.data, y->text.data, x->text.length));
        assert(x->bounds.x == y->bounds.x && x->bounds.y == y->bounds.y &&
               x->bounds.w == y->bounds.w && x->bounds.h == y->bounds.h &&
               x->font_size == y->font_size);
    }
}

static void badge_visuals(void) {
    cui_bridge_app *app = make_app();
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_badge_config_v1 cfg = defaults();
    cui_bridge_node badge;
    assert(cui_bridge_badge_create_v1(app, root, "marker", &cfg, NULL, NULL, 0, NULL, 0, &badge) ==
           CUI_BRIDGE_OK);
    cui_bridge_node button = create(app, badge, CUI_BRIDGE_BUTTON, "inbox");
    assert(cui_bridge_set_text(app, button, "Inbox") == CUI_BRIDGE_OK);
    cui_bridge_icon_handle_v1 owned = art(app);
    cui_context *oracle =
        cui_create((cui_config){.width = 320, .height = 144, .measure = bridge_measure});
    cui_software *reference = cui_software_create(320, 144, NULL);
    cui_icon_state icon = {0};
    assert(oracle && reference && cui_icon_set_data(&icon, cui_string(svg)));
    cui_bridge_rect content = {0};
    for (unsigned mode = 0; mode < 7; ++mode) {
        cfg = defaults();
        cfg.size = mode % 4;
        const char *named = NULL;
        const char *name = "";
        cui_bridge_icon_options_v1 picture = {.struct_size = sizeof picture, .schema_version = 1};
        if (mode == 1) {
            cfg.count = 3;
            cfg.maximum = 0;
        } else if (mode == 2) {
            cfg.count = cfg.maximum = SIZE_MAX;
            cfg.design_size = CUI_BRIDGE_SIZE_MD;
        } else if (mode == 3) {
            cfg.variant = CUI_BRIDGE_BADGE_DOT_V1;
            cfg.flags = CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1;
        } else if (mode == 4) {
            cfg.variant = CUI_BRIDGE_BADGE_ICON_V1;
            named = "check";
        } else if (mode >= 5) {
            cfg.variant = CUI_BRIDGE_BADGE_ICON_V1;
            cfg.artwork = owned;
            cfg.design_size = CUI_BRIDGE_SIZE_XL;
            picture.size = CUI_ICON_CUSTOM;
            picture.custom_size = 14;
            picture.flags = CUI_BRIDGE_ICON_COLOR_V1 | CUI_BRIDGE_ICON_TRANSFORM_V1 |
                            CUI_BRIDGE_ICON_DEBUG_PLACEHOLDER_V1;
            picture.color_rgba = UINT32_C(0x239acaff);
            picture.transform[1] = 1;
            picture.transform[2] = -1;
            picture.transform[4] = 2;
            name = "Inbox status";
            if (mode == 6) {
                static const char path[] = "missing-icon.svg";
                assert(cui_bridge_icon_set_source_v1(app, owned, CUI_BRIDGE_ICON_PATH_V1, path,
                                                     sizeof path - 1) == CUI_BRIDGE_OK);
                assert(cui_icon_set_path(&icon, cui_string(path), NULL, NULL));
            }
        }
        assert(cui_bridge_badge_configure_v1(app, badge, &cfg, mode >= 5 ? &picture : NULL, name,
                                             (uint32_t)strlen(name), named,
                                             named ? 5 : 0) == CUI_BRIDGE_OK);
        (void)result(app, badge, CUI_BRIDGE_EMPTY);
        cui_bridge_status step = cui_bridge_step(app, .016f);
        if (step != CUI_BRIDGE_OK)
            fprintf(stderr, "Badge rendering mode %u: step=%d core=%d\n", mode, step,
                    cui_last_error(app->ui));
        assert(step == CUI_BRIDGE_OK);
        const cui_bridge_badge_result_v1 r = result(app, badge, CUI_BRIDGE_OK);
        assert(r.flags == (mode >= 5 ? CUI_BRIDGE_BADGE_ICON_OBSERVED_V1 : 0u));
        assert(r.icon_error == (mode == 6 ? CUI_ICON_NOT_FOUND : CUI_ICON_OK));
        assert(r.frame_status == CUI_BRIDGE_OK);
        cui_bridge_rect current = bounds(app, button);
        if (!mode)
            content = current;
        assert(current.x == content.x && current.y == content.y && current.width == content.width &&
               current.height == content.height);

        assert(cui_begin_frame(oracle, 320, 144, .016f));
        cui_begin_box(oracle, 95,
                      (cui_box){.width = -1, .height = -1, .background = app->theme.background});
        cui_begin_box(oracle, 96, (cui_box){.width = -1, .height = -1, .padding = 16});
        cui_badge_options expected = {.variant = (cui_badge_variant)cfg.variant,
                                      .size = (cui_control_size)cfg.size,
                                      .design_size = (cui_design_size)cfg.design_size,
                                      .count = (size_t)cfg.count,
                                      .maximum = (size_t)cfg.maximum,
                                      .name = cui_string(name),
                                      .icon = named ? cui_string(named) : (cui_str){0},
                                      .custom_color = cfg.flags != 0};
        if (mode >= 5) {
            cui_badge_icon_content artwork = {.state = &icon,
                                              .options = {.size = CUI_ICON_CUSTOM,
                                                          .custom_size = 14,
                                                          .custom_color = true,
                                                          .color = {0x23, 0x9a, 0xca, 0xff},
                                                          .custom_transform = true,
                                                          .transform = {0, 1, -1, 0, 2, 0},
                                                          .debug_placeholder = true}};
            cui_badge_result native = cui_badge_with_icon(oracle, 97, &app->theme, expected,
                                                          &artwork, oracle_child, &app->theme);
            assert(native.submitted && (uint32_t)native.icon_error == r.icon_error);
        } else {
            assert(cui_badge_ex(oracle, 97, &app->theme, expected, oracle_child, &app->theme));
        }
        cui_end_box(oracle);
        cui_end_box(oracle);
        cui_draw_list list = cui_end_frame(oracle);
        compare_commands(bridge_draw, list);
        assert(draw_shapes(reference, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h, ow, oh;
        const unsigned char *current_rgba = cui_software_pixels(software, &w, &h);
        const unsigned char *oracle_rgba = cui_software_pixels(reference, &ow, &oh);
        assert(w == ow && h == oh && !memcmp(current_rgba, oracle_rgba, (size_t)w * h * 4));
        cui_bridge_rect b = {.x = -999};
        assert(cui_bridge_badge_indicator_bounds_v1(app, badge, &b) ==
               (mode ? CUI_BRIDGE_OK : CUI_BRIDGE_EMPTY));
        char numeric[32];
        snprintf(numeric, sizeof numeric, "%zu", SIZE_MAX);
        inspect_semantics(app,
                          mode == 1   ? "0+"
                          : mode == 2 ? numeric
                          : mode == 3 ? "Notification"
                          : mode == 4 ? "Status"
                                      : name,
                          mode != 0);
        if (mode) {
            cui_rect direct = cui_bounds(oracle, cui_badge_indicator_id(97));
            assert(b.x == direct.x && b.y == direct.y && b.width == direct.w &&
                   b.height == direct.h);
        } else {
            assert(b.x == -999);
        }
    }
    click(app, button, true); /* Artwork failure still submits and routes the child. */
    assert(cui_bridge_icon_set_source_v1(app, owned, CUI_BRIDGE_ICON_DATA_V1, svg,
                                         sizeof svg - 1) == CUI_BRIDGE_OK);
    cui_bridge_node standalone = create(app, root, CUI_BRIDGE_ICON, "standalone");
    cui_bridge_icon_options_v1 presentation = {.struct_size = sizeof presentation,
                                               .schema_version = 1};
    assert(cui_bridge_icon_configure_node_v1(app, standalone, owned, &presentation, NULL, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(result(app, badge, CUI_BRIDGE_OK).icon_error == CUI_ICON_OK);
    cui_bridge_icon_result_v1 icon_result = {.struct_size = sizeof icon_result,
                                             .schema_version = 1};
    assert(cui_bridge_icon_get_node_result_v1(app, standalone, &icon_result) == CUI_BRIDGE_OK &&
           icon_result.icon_error == CUI_ICON_OK);
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, owned, &info) == CUI_BRIDGE_OK &&
           info.reference_count == 2);
    assert(cui_bridge_icon_destroy_v1(app, owned) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, badge) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, owned) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, standalone) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(app, owned) == CUI_BRIDGE_OK);
    cui_icon_state_destroy(&icon);
    cui_software_destroy(reference);
    cui_destroy(oracle);
    cui_bridge_destroy(app);
}

static void badge_admission(void) {
    cui_bridge_app *app = make_app();
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node panel = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "panel");
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    cui_bridge_node old = create(app, panel, CUI_BRIDGE_BADGE, "legacy-pill");
    assert(cui_bridge_set_text(app, old, "New") == CUI_BRIDGE_OK);
    cui_bridge_node badge = {UINT32_MAX, UINT32_MAX};
    assert(cui_bridge_node_create(app, old, CUI_BRIDGE_BUTTON, "denied", &badge) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, panel, CUI_BRIDGE_BADGE_CONFIGURED, "denied", &badge) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_badge_config_v1 cfg = defaults();
    cfg.count = 8;
    for (unsigned i = 1; i <= 3; ++i) {
        allocations_until_failure = i;
        assert(cui_bridge_badge_create_v1(app, panel, "outer", &cfg, NULL, NULL, 0, NULL, 0,
                                          &badge) == CUI_BRIDGE_OUT_OF_MEMORY);
        assert(!app->icons && badge.slot == UINT32_MAX);
    }
    char name[] = "Unread";
    assert(cui_bridge_badge_create_v1(app, panel, "outer", &cfg, NULL, name, 6, NULL, 0, &badge) ==
           CUI_BRIDGE_OK);
    memset(name, 'X', 6);
    assert(!app->icons->slots);
    cui_bridge_node nested;
    cfg.variant = CUI_BRIDGE_BADGE_DOT_V1;
    assert(cui_bridge_badge_create_v1(app, badge, "nested", &cfg, NULL, NULL, 0, NULL, 0,
                                      &nested) == CUI_BRIDGE_OK);
    cui_bridge_node button = create(app, nested, CUI_BRIDGE_BUTTON, "open");
    assert(cui_bridge_set_text(app, button, "Open") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, badge, "wrong") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    click(app, button, true);
    assert(cui_bridge_set_disabled(app, badge, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    click(app, button, false);
    assert(cui_bridge_set_disabled(app, badge, 0) == CUI_BRIDGE_OK);

    uint64_t observed = result(app, badge, CUI_BRIDGE_OK).frame_sequence;
    cui_bridge_icon_options_v1 artwork = {.struct_size = sizeof artwork, .schema_version = 1};
    assert(cui_bridge_badge_configure_v1(app, badge, &cfg, &artwork, NULL, 0, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    const char invalid[] = {(char)0xff};
    assert(cui_bridge_badge_configure_v1(app, badge, &cfg, NULL, invalid, 1, NULL, 0) ==
           CUI_BRIDGE_INVALID_UTF8);
    allocations_until_failure = 1;
    assert(cui_bridge_badge_configure_v1(app, badge, &cfg, NULL, NULL, 0, NULL, 0) ==
           CUI_BRIDGE_OUT_OF_MEMORY);
    assert(result(app, badge, CUI_BRIDGE_OK).frame_sequence == observed);
    assert(cui_bridge_badge_get_config_v1(app, badge, &cfg, &artwork) == CUI_BRIDGE_OK);
    assert(cfg.variant == CUI_BRIDGE_BADGE_COUNT_V1 && cfg.count == 8 && !artwork.flags);
    uint32_t required = 0;
    char text[16] = "";
    assert(cui_bridge_badge_copy_text_v1(app, badge, CUI_BRIDGE_BADGE_NAME_V1, text, 3,
                                         &required) == CUI_BRIDGE_CAPACITY);
    assert(required == 7 && !text[0]);
    assert(cui_bridge_badge_copy_text_v1(app, badge, CUI_BRIDGE_BADGE_NAME_V1, text, sizeof text,
                                         &required) == CUI_BRIDGE_OK);
    assert(!strcmp(text, "Unread"));
    app->width = 260;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    (void)result(app, badge, CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    (void)result(app, badge, CUI_BRIDGE_EMPTY);
    (void)result(app, nested, CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR);
    assert(result(app, nested, CUI_BRIDGE_OK).frame_status == CUI_BRIDGE_CORE_ERROR);
    render_status = CUI_BRIDGE_OK;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    click(app, button, true);
    assert(cui_bridge_node_remove(app, panel) == CUI_BRIDGE_OK);
    (void)result(app, badge, CUI_BRIDGE_STALE_HANDLE);
    (void)result(app, nested, CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}

int main(void) {
    assert(cui_bridge_badge_versions() == 1);
    assert(cui_bridge_badge_record_size_v1(CUI_BRIDGE_BADGE_CONFIG_RECORD_V1) == 64);
    assert(cui_bridge_badge_record_size_v1(CUI_BRIDGE_BADGE_RESULT_RECORD_V1) == 40);
    assert(cui_bridge_badge_record_size_v1(2) == 0 && cui_bridge_icon_record_size_v1(6) == 0);
    badge_visuals();
    badge_admission();
}
