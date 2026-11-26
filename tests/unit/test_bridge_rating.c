/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

static const char outline_svg[] =
    "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'>"
    "<rect x='2' y='2' width='12' height='12' stroke='white' stroke-width='2' fill='none'/>"
    "</svg>";
static const char fill_svg[] = "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'>"
                               "<rect width='16' height='16' fill='white'/></svg>";

static cui_bridge_icon_handle_v1 icon(cui_bridge_app *app, const char *svg) {
    cui_bridge_icon_handle_v1 handle;
    cui_bridge_icon_limits_v1 limits = {.struct_size = sizeof limits, .schema_version = 1};
    assert(cui_bridge_icon_create_v1(app, &limits, &handle) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_set_source_v1(app, handle, CUI_BRIDGE_ICON_DATA_V1, svg,
                                         (uint32_t)strlen(svg)) == CUI_BRIDGE_OK);
    return handle;
}

static uint32_t references(cui_bridge_app *app, cui_bridge_icon_handle_v1 handle) {
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(app, handle, &info) == CUI_BRIDGE_OK);
    return info.reference_count;
}

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static cui_draw_list inspect_paint(cui_bridge_app *app) {
    assert(cui_begin_frame(app->ui, app->width, app->height, .016f));
    cui_begin_box(app->ui, 1, (cui_box){.width = -1, .height = -1});
    bridge_render_node(app, app->root);
    cui_end_box(app->ui);
    cui_draw_list list = cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK);
    return list;
}

static cui_bridge_rect part(cui_bridge_app *app, cui_bridge_node rating, uint32_t id) {
    cui_bridge_rect bounds;
    assert(cui_bridge_numeric_part_bounds(app, rating, id, &bounds) == CUI_BRIDGE_OK);
    return bounds;
}

static void click(cui_bridge_app *app, cui_bridge_rect bounds, float fraction) {
    float x = bounds.x + fraction * bounds.width, y = bounds.y + bounds.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    step(app);
}

static void expect_value(cui_bridge_app *app, cui_bridge_node rating, double expected,
                         bool changed) {
    double value;
    assert(cui_bridge_get_value(app, rating, &value) == CUI_BRIDGE_OK && value == expected);
    cui_bridge_notification_v1 event;
    char text[8];
    uint32_t capacity;
    if (changed) {
        assert(cui_bridge_poll_value_notification_v1(app, sizeof event, CUI_BRIDGE_VALUE_SCALAR_V1,
                                                     &event, text, sizeof text,
                                                     &capacity) == CUI_BRIDGE_OK);
        assert(event.kind == CUI_BRIDGE_EVENT_CHANGE && event.node_kind == CUI_BRIDGE_RATING);
        assert(event.node.slot == rating.slot && event.node.generation == rating.generation);
        assert(event.payload_kind == CUI_BRIDGE_PAYLOAD_SCALAR &&
               event.payload.scalar_value == expected && event.legacy.value == expected);
    }
    assert(cui_bridge_poll_value_notification_v1(app, sizeof event, CUI_BRIDGE_VALUE_SCALAR_V1,
                                                 &event, text, sizeof text,
                                                 &capacity) == CUI_BRIDGE_EMPTY);
}

static uint32_t tint(cui_bitmap_paint paint) {
    cui_color c = paint.tint;
    return ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) | ((uint32_t)c.b << 8) | c.a;
}

static void check_paint(cui_bridge_app *app, const cui_bridge_rating_config_v1 *config,
                        bool preview, bool outline_fallback) {
    cui_draw_list list = inspect_paint(app);
    const bridge_icon_owned *outline = app->icons->slots[config->outline_icon.slot - 1].value;
    const bridge_icon_owned *fill = app->icons->slots[config->fill_icon.slot - 1].value;
    cui_bitmap outline_pixels = cui_icon_state_bitmap(&outline->state);
    cui_bitmap fill_pixels = cui_icon_state_bitmap(&fill->state);
    assert(cui_bitmap_valid(fill_pixels));
    assert(cui_bitmap_valid(outline_pixels) == !outline_fallback);
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(app->ui, &semantic_count);
    size_t ratings = 0;
    for (size_t i = 0; i < semantic_count; ++i) {
        if (semantics[i].role == CUI_ROLE_SLIDER) {
            assert(semantics[i].value == 1.5f);
            ++ratings;
        }
    }
    assert(ratings == 1);
    unsigned outlines = 0, fills = 0, halves = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *command = &list.commands[i];
        if (command->type != CUI_DRAW_IMAGE)
            continue;
        cui_bitmap_paint paint = command->bitmap;
        assert(paint.custom_tint && command->bounds.h == 18);
        if (paint.bitmap.pixels == fill_pixels.pixels) {
            assert(tint(paint) == config->fill_rgba);
            if (paint.source.w * 2 == fill_pixels.width && command->bounds.w == 9)
                ++halves;
            ++fills;
        } else {
            assert(outline_fallback || paint.bitmap.pixels == outline_pixels.pixels);
            assert(tint(paint) == (outlines == 0 ? config->fill_rgba
                                   : preview     ? config->preview_rgba
                                                 : config->outline_rgba));
            ++outlines;
        }
    }
    assert(fills == 2 && halves == 1 && outlines == 5);
}

int main(void) {
    cui_bridge_status status;
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 800,
                                  .height = 400};
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, rating;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RATING, "rating", &rating) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, rating, "Rate delivery") == CUI_BRIDGE_OK);
    assert(cui_bridge_rating_config_size_v1() == 48);
    cui_bridge_rating_config_v1 config = {
        .struct_size = sizeof config,
        .schema_version = 1,
        .flags = CUI_BRIDGE_RATING_HALF_STEPS_V1 | CUI_BRIDGE_RATING_HOVER_PREVIEW_V1 |
                 CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1 | CUI_BRIDGE_RATING_CLEARABLE_V1 |
                 CUI_BRIDGE_RATING_CUSTOM_FILL_V1 | CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1 |
                 CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1,
        .design_size = CUI_BRIDGE_SIZE_LG,
        .width = 300,
        .fill_rgba = UINT32_C(0xce4275ff),
        .outline_rgba = UINT32_C(0x54775cff),
        .preview_rgba = UINT32_C(0x245cb6ff),
        .outline_icon = icon(app, outline_svg),
        .fill_icon = icon(app, fill_svg)};
    cui_bridge_rating_config_v1 read = {.struct_size = sizeof read, .schema_version = 1};
    assert(cui_bridge_rating_get_config_v1(app, rating, &read) == CUI_BRIDGE_EMPTY);
    assert(!read.width && !read.flags);
    assert(cui_bridge_rating_configure_v1(app, root, &config) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_OK);
    assert(references(app, config.outline_icon) == 1 && references(app, config.fill_icon) == 1);
    assert(cui_bridge_icon_destroy_v1(app, config.fill_icon) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_rating_config_v1 bad = config;
    ++bad.fill_icon.generation;
    assert(cui_bridge_rating_configure_v1(app, rating, &bad) == CUI_BRIDGE_STALE_HANDLE);
    bad = config;
    bad.outline_icon.generation = 0;
    assert(cui_bridge_rating_configure_v1(app, rating, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    bad.flags |= 1u << 8;
    assert(cui_bridge_rating_configure_v1(app, rating, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    --bad.struct_size;
    assert(cui_bridge_rating_configure_v1(app, rating, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    bad.width = NAN;
    assert(cui_bridge_rating_configure_v1(app, rating, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_rating_get_config_v1(app, rating, &read) == CUI_BRIDGE_OK);
    assert(read.width == config.width && read.flags == config.flags &&
           read.fill_icon.generation == config.fill_icon.generation);
    assert(references(app, config.outline_icon) == 1 && references(app, config.fill_icon) == 1);

    assert(cui_bridge_set_value(app, rating, 1.5) == CUI_BRIDGE_OK);
    step(app);
    expect_value(app, rating, 1.5, false);
    cui_bridge_rect bounds;
    assert(cui_bridge_node_bounds(app, rating, &bounds) == CUI_BRIDGE_OK && bounds.width == 300);
    check_paint(app, &config, false, false);
    cui_bridge_rect last = part(app, rating, 14);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, last.x + last.width / 2,
                                    last.y + last.height / 2) == CUI_BRIDGE_OK);
    step(app);
    expect_value(app, rating, 1.5, false);
    check_paint(app, &config, true, false);
    assert(cui_bridge_icon_set_source_v1(app, config.outline_icon, CUI_BRIDGE_ICON_EMPTY_V1, NULL,
                                         0) == CUI_BRIDGE_OK);
    check_paint(app, &config, true, true);

    click(app, part(app, rating, 13), .25f);
    expect_value(app, rating, 3.5, true);
    click(app, part(app, rating, 13), .25f);
    expect_value(app, rating, 3, true);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    expect_value(app, rating, 3.5, true);
    uint32_t flags = config.flags;
    config.flags |= CUI_BRIDGE_RATING_READONLY_V1;
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_OK);
    step(app);
    click(app, part(app, rating, 14), .75f);
    expect_value(app, rating, 3.5, false);
    config.flags = flags;
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(app, rating, 1) == CUI_BRIDGE_OK);
    step(app);
    click(app, part(app, rating, 14), .75f);
    expect_value(app, rating, 3.5, false);
    assert(cui_bridge_set_disabled(app, rating, 0) == CUI_BRIDGE_OK);
    step(app);
    click(app, part(app, rating, 1), .5f);
    expect_value(app, rating, 0, true);

    cui_bridge_icon_handle_v1 old_outline = config.outline_icon;
    config.outline_icon = config.fill_icon;
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_OK);
    assert(references(app, old_outline) == 0 && references(app, config.fill_icon) == 2);
    assert(cui_bridge_icon_destroy_v1(app, old_outline) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, rating, 2.5) == CUI_BRIDGE_OK);
    assert(cui_bridge_rating_configure_v1(app, rating, NULL) == CUI_BRIDGE_OK);
    assert(references(app, config.fill_icon) == 0);
    expect_value(app, rating, 2.5, false);
    step(app);
    expect_value(app, rating, 3, true);
    assert(cui_bridge_numeric_part_bounds(app, rating, 1, &bounds) == CUI_BRIDGE_INVALID_STATE);
    click(app, part(app, rating, 12), .2f);
    expect_value(app, rating, 3, false);
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, rating) == CUI_BRIDGE_OK);
    assert(references(app, config.fill_icon) == 0);
    assert(cui_bridge_rating_configure_v1(app, rating, &config) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_icon_destroy_v1(app, config.fill_icon) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
