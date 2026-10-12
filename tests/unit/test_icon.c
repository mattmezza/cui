/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static const char asymmetric[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
    "<rect x=\"2\" y=\"3\" width=\"6\" height=\"10\" fill=\"red\"/></svg>";
static const char wide[] = "<svg width=\"24\" height=\"12\" viewBox=\"0 0 24 12\">"
                           "<rect x=\"2\" y=\"2\" width=\"20\" height=\"8\" fill=\"black\"/></svg>";
static unsigned alpha(cui_bitmap bitmap, unsigned x, unsigned y) {
    assert(x < bitmap.width && y < bitmap.height);
    return bitmap.pixels[y * bitmap.stride + x * 4 + 3];
}
static size_t covered(cui_bitmap bitmap) {
    size_t count = 0;
    for (unsigned y = 0; y < bitmap.height; y++)
        for (unsigned x = 0; x < bitmap.width; x++)
            count += alpha(bitmap, x, y) > 0;
    return count;
}
static void centroid(cui_bitmap bitmap, float *x, float *y) {
    float sum = 0, sx = 0, sy = 0;
    for (unsigned j = 0; j < bitmap.height; j++)
        for (unsigned i = 0; i < bitmap.width; i++) {
            float value = alpha(bitmap, i, j);
            sum += value;
            sx += (i + .5f) * value;
            sy += (j + .5f) * value;
        }
    assert(sum > 0);
    *x = sx / sum;
    *y = sy / sum;
}
static cui_draw_list scene(cui_context *ui, cui_icon_state *state, cui_icon_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.row = true});
    assert(cui_icon(ui, 10, &theme, state, options) == CUI_ICON_OK);
    cui_begin_box(ui, 20, (cui_box){.width = 20, .height = 20});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void test_assets(void) {
    assert(cui_icon_catalog_count() == 1830);
    cui_icon_state state;
    cui_icon_state_init(&state);
    size_t defaults = 0, embedded = 0;
    for (size_t i = 0; i < cui_icon_catalog_count(); i++) {
        cui_str name = cui_icon_catalog_name(i), path = cui_icon_catalog_path(i), svg;
        assert(name.length && path.length > name.length && !memcmp(path.data, "icons/", 6));
        defaults += cui_icon_catalog_default(i);
        if (!cui_icon_asset(name, &svg)) {
            assert(!cui_icon_catalog_default(i));
            continue;
        }
        embedded++;
        assert(svg.length && cui_icon_set_name(&state, name, NULL, NULL));
        assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
        assert(cui_bitmap_valid(cui_icon_state_bitmap(&state)) &&
               covered(cui_icon_state_bitmap(&state)));
        cui_icon_state_collect(&state);
        assert(state.cache_versions == 1);
    }
    assert(defaults == 106);
#if defined(CUI_ICON_ALL_ASSETS) && CUI_ICON_ALL_ASSETS
    assert(embedded == 1830);
#else
    assert(embedded == 106);
#endif
    assert(!cui_icon_catalog_name(1830).length && !cui_icon_catalog_path(1830).length);
    assert(!cui_icon_asset(cui_string("missing"), &(cui_str){0}));
    cui_icon_state_destroy(&state);
}
static void test_source_and_raster(void) {
    cui_icon_state state = {0};
    char owned[sizeof(asymmetric)];
    memcpy(owned, asymmetric, sizeof(owned));
    assert(cui_icon_set_data(&state, cui_string(owned)));
    memset(owned, 'x', sizeof(owned));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    cui_bitmap original = cui_icon_state_bitmap(&state);
    assert(original.width == 24 && original.height == 24 && alpha(original, 4, 5) == 255 &&
           alpha(original, 20, 20) == 0);
    float x, y;
    centroid(original, &x, &y);
    assert(fabsf(x - 5) < .05f && fabsf(y - 8) < .05f);
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_rotation(3.14159265358979323846f / 2)) ==
           CUI_ICON_OK);
    cui_bitmap rotated = cui_icon_state_bitmap(&state);
    cui_rect bounds = cui_icon_state_paint_bounds(&state);
    centroid(rotated, &x, &y);
    assert(fabsf(x + bounds.x - 16) < .15f && fabsf(y + bounds.y - 5) < .15f);
    assert(state.parse_count == 1 && state.raster_count == 2 && alpha(original, 4, 5) == 255);
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    assert(cui_icon_state_bitmap(&state).pixels == original.pixels && state.raster_count == 2);
    assert(cui_icon_set_data(&state, cui_string(wide)));
    assert(cui_icon_prepare(&state, 24, 40, cui_icon_identity()) == CUI_ICON_OK);
    bounds = cui_icon_state_paint_bounds(&state);
    assert(bounds.x == 0 && bounds.y == 14 && bounds.w == 24 && bounds.h == 12);
    assert(state.parse_count == 2 && alpha(original, 4, 5) == 255);
    cui_icon_state_collect(&state);
    assert(state.cache_versions == 1);
    assert(cui_icon_set_empty(&state));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    assert(!cui_bitmap_valid(cui_icon_state_bitmap(&state)));
    cui_icon_state_collect(&state);
    assert(!state.cache_versions && !state.cache_bytes);
    cui_icon_state_destroy(&state);
}
typedef struct {
    unsigned calls;
    bool available;
    const char *svg;
} loader_state;
static bool load(void *user, cui_str path, cui_str *svg) {
    loader_state *loader = user;
    assert(path.length);
    loader->calls++;
    if (!loader->available)
        return false;
    *svg = cui_string(loader->svg);
    return true;
}
static void test_tiny_round_caps(void) {
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state,
                             cui_string("<svg width=\"24\" height=\"24\"><path d=\"M8 20h.01\" "
                                        "fill=\"none\" stroke=\"black\" stroke-width=\"2\" "
                                        "stroke-linecap=\"round\"/></svg>")));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    cui_bitmap bitmap = cui_icon_state_bitmap(&state);
    assert(covered(bitmap) >= 4 && alpha(bitmap, 8, 20) > 0);
    cui_icon_state_destroy(&state);
}
static void test_loader(void) {
    cui_icon_state state = {0};
    loader_state loader = {0, true, asymmetric};
    assert(cui_icon_set_path(&state, cui_string("custom.svg"), load, &loader));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    assert(loader.calls == 1);
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK &&
           loader.calls == 1);
    loader.svg = wide;
    cui_icon_reload(&state);
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK &&
           loader.calls == 2);
    loader.available = false;
    assert(cui_icon_set_name(&state, cui_string("check"), load, &loader));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_OK);
    assert(cui_icon_set_path(&state, cui_string("not-present.svg"), NULL, NULL));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_NOT_FOUND);
    assert(!cui_icon_set_name(&state, cui_string("not-present"), NULL, NULL));
    cui_icon_state_destroy(&state);
}
static void test_component_sizes_color_and_lifetime(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 200});
    assert(ui);
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state, cui_string(asymmetric)));
    const float sizes[] = {16, 12, 14, 16, 24, 15.25f};
    for (int size = CUI_ICON_INHERITED; size <= CUI_ICON_CUSTOM; size++) {
        cui_draw_list draw = scene(
            ui, &state, (cui_icon_options){.size = (cui_icon_size)size, .custom_size = 15.25f});
        assert(cui_bounds(ui, 10).w == sizes[size] && cui_bounds(ui, 10).h == sizes[size] &&
               cui_bounds(ui, 20).x == sizes[size]);
        size_t semantic_count;
        cui_semantics(ui, &semantic_count);
        assert(!semantic_count);
        bool image = false;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_IMAGE) {
                image = true;
                assert(draw.commands[i].bitmap.custom_tint);
            }
        assert(image);
        cui_icon_state_collect(&state);
    }
    cui_icon_options options = {.inherited_text_size = 21,
                                .custom_color = true,
                                .color = {12, 34, 56, 123},
                                .custom_opacity = true,
                                .opacity = .5f};
    cui_draw_list draw = scene(ui, &state, options);
    assert(cui_bounds(ui, 10).w == 21);
    const unsigned char *pixels = cui_icon_state_bitmap(&state).pixels;
    size_t rasters = state.raster_count, parses = state.parse_count;
    options.color = (cui_color){23, 45, 67, 231};
    options.opacity = .25f;
    draw = scene(ui, &state, options);
    assert(state.raster_count == rasters && state.parse_count == parses &&
           cui_icon_state_bitmap(&state).pixels == pixels);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE)
            assert(draw.commands[i].bitmap.tint.r == 23 && draw.commands[i].bitmap.tint.a == 231 &&
                   draw.commands[i].bitmap.opacity == .25f);
    options.custom_width = options.custom_height = true;
    options.width = 24;
    options.height = 12;
    options.custom_transform = true;
    options.transform = (cui_icon_transform){2, 0, 0, 1, 3, 0};
    draw = scene(ui, &state, options);
    assert(cui_bounds(ui, 10).w == 24 && cui_bounds(ui, 10).h == 12 && cui_bounds(ui, 20).x == 24);
    bool overflow = false;
    for (size_t i = 0; i < draw.count; i++)
        overflow |= draw.commands[i].type == CUI_DRAW_IMAGE && draw.commands[i].bounds.w == 48 &&
                    draw.commands[i].bounds.x == -9 && draw.commands[i].bounds.y == -6;
    assert(overflow);
    options = (cui_icon_options){.custom_width = true, .width = 30};
    draw = scene(ui, &state, options);
    assert(cui_bounds(ui, 10).w == 30 && cui_bounds(ui, 10).h == 0);
    for (size_t i = 0; i < draw.count; i++)
        assert(draw.commands[i].type != CUI_DRAW_IMAGE);
    options.size = CUI_ICON_LARGE;
    scene(ui, &state, options);
    assert(cui_bounds(ui, 10).w == 24 && cui_bounds(ui, 10).h == 24);
    assert(cui_icon_set_empty(&state));
    draw = scene(ui, &state, (cui_icon_options){.inherited_text_size = 19});
    assert(cui_bounds(ui, 10).w == 19 && cui_bounds(ui, 10).h == 19);
    for (size_t i = 0; i < draw.count; i++)
        assert(draw.commands[i].type != CUI_DRAW_IMAGE);
    assert(cui_icon_set_data(&state, cui_string(asymmetric)));
    /* Two source replacements in one frame keep both borrowed masks immutable. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_theme theme = cui_theme_light();
    assert(cui_icon_set_data(&state, cui_string(asymmetric)));
    assert(cui_icon(ui, 30, &theme, &state, (cui_icon_options){.size = CUI_ICON_LARGE}) ==
           CUI_ICON_OK);
    cui_bitmap first = cui_icon_state_bitmap(&state);
    assert(cui_icon_set_data(&state, cui_string(wide)));
    assert(cui_icon(ui, 40, &theme, &state, (cui_icon_options){.size = CUI_ICON_LARGE}) ==
           CUI_ICON_OK);
    draw = cui_end_frame(ui);
    unsigned images = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            images++;
            assert(cui_bitmap_valid(draw.commands[i].bitmap.bitmap));
            assert(covered(draw.commands[i].bitmap.bitmap));
        }
    assert(images == 2 && alpha(first, 4, 5) == 255 && cui_last_error(ui) == CUI_OK);
    cui_icon_state_collect(&state);
    theme.font_size = 20;
    assert(cui_begin_frame(ui, 300, 200, .016f));
    assert(cui_icon(ui, 50, &theme, &state,
                    (cui_icon_options){.size = CUI_ICON_SMALL,
                                       .inherited_text_size = 30,
                                       .custom_inherited_color = true,
                                       .inherited_color = {91, 92, 93, 94}}) == CUI_ICON_OK);
    draw = cui_end_frame(ui);
    assert(cui_bounds(ui, 50).w == 17.5f && cui_bounds(ui, 50).h == 17.5f);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE)
            assert(draw.commands[i].bitmap.tint.r == 91 && draw.commands[i].bitmap.tint.a == 94);
    cui_icon_state_destroy(&state);
    cui_destroy(ui);
}
static void test_invalid_and_limits(void) {
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state, cui_string("<svg><path/>")));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_PARSE);
    assert(cui_icon_set_data(&state, cui_string("<svg width=\"0\" height=\"24\"></svg>")));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_PARSE);
    assert(!cui_icon_set_data(&state, (cui_str){"a\0b", 3}));
    /* Reject oversized spans before touching their purported input bytes. */
    assert(!cui_icon_set_data(&state, (cui_str){"x", SIZE_MAX}));
    assert(state.error == CUI_ICON_LIMIT);
    assert(!cui_icon_set_path(&state, (cui_str){"x", 4097}, NULL, NULL));
    assert(state.error == CUI_ICON_LIMIT);
    assert(cui_icon_set_data(&state, cui_string(asymmetric)));
    assert(cui_icon_prepare(&state, NAN, 24, cui_icon_identity()) == CUI_ICON_INVALID);
    assert(cui_icon_prepare(&state, 24, 24, (cui_icon_transform){NAN, 0, 0, 1, 0, 0}) ==
           CUI_ICON_INVALID);
    assert(cui_icon_set_limits(&state, (cui_icon_limits){.max_pixels = 256, .max_versions = 1}));
    assert(cui_icon_prepare(&state, 24, 24, cui_icon_identity()) == CUI_ICON_LIMIT);
    assert(cui_icon_prepare(&state, 12, 12, cui_icon_identity()) == CUI_ICON_OK);
    cui_bitmap first = cui_icon_state_bitmap(&state);
    assert(cui_icon_prepare(&state, 14, 14, cui_icon_identity()) == CUI_ICON_LIMIT);
    assert(covered(first) && state.cache_versions == 1);
    assert(cui_icon_set_empty(&state));
    assert(cui_icon_prepare(&state, 12, 12, cui_icon_identity()) == CUI_ICON_OK);
    cui_icon_state_collect(&state);
    assert(cui_icon_set_data(&state, cui_string(asymmetric)));
    assert(cui_icon_prepare(&state, 14, 14, cui_icon_identity()) == CUI_ICON_OK);
    assert(cui_icon_prepare(&state, 14, 14, (cui_icon_transform){0}) == CUI_ICON_OK);
    assert(!cui_bitmap_valid(cui_icon_state_bitmap(&state)));
    cui_icon_state_destroy(&state);
}
int main(void) {
    test_assets();
    test_source_and_raster();
    test_loader();
    test_tiny_round_caps();
    test_component_sizes_color_and_lifetime();
    test_invalid_and_limits();
    return 0;
}
