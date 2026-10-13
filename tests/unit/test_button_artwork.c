/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static cui_measure_fn fixture_measure;
static void *fixture_measure_user;
static cui_draw_list scene(cui_context *ui, cui_theme *t, cui_button_options o, unsigned passive) {
    assert(cui_begin_frame(ui, 2000, 1200, .1));
    cui_begin_box(
        ui, 1,
        (cui_box){
            .width = 1900, .height = 1100, .inert = passive == 1, .layout_only = passive == 2});
    (void)cui_button_ex(ui, 10, t, cui_string("Artwork"), o);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
static const cui_draw_command *image(cui_draw_list list) {
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_IMAGE)
            return &list.commands[i];
    return NULL;
}
static bool selected[2];
static void select_item(void *user, const cui_button_group_request *r) {
    (void)user;
    selected[0] = selected[1] = false;
    if (r->item_index < 2)
        selected[r->item_index] = true;
}
static cui_button_artwork *mutable_art;
static unsigned clicks;
static void change_descriptor(void *user) {
    (void)user;
    clicks++;
    mutable_art->kind = CUI_BUTTON_ARTWORK_PROGRESS_CIRCLE;
    mutable_art->progress = NULL;
}
static unsigned children;
static void child(cui_context *ui, void *user) {
    (void)user;
    children++;
    cui_text(ui, cui_string("Custom"), 16, (cui_color){255, 0, 0, 255});
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 65536,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    cui_icon_state icon;
    cui_icon_state_init(&icon);
    assert(cui_icon_set_data(
        &icon, cui_string("<svg viewBox='0 0 16 16'><path d='M2 2h12v12H2z'/></svg>")));
    cui_button_artwork_result result = {0};
    cui_button_artwork art = {.kind = CUI_BUTTON_ARTWORK_ICON,
                              .icon = &icon,
                              .icon_options = {.size = CUI_ICON_CUSTOM,
                                               .custom_size = 300,
                                               .custom_color = true,
                                               .color = {231, 24, 91, 255}},
                              .result = &result};
    cui_theme theme = cui_theme_light();
    for (unsigned density = 0; density < 2; density++) {
        theme.font_size = density ? 14 : 16;
        for (unsigned size = 0; size < 18; size++) {
            cui_button_options o = {.leading_artwork = &art, .icon_size = 24};
            if (size < 4)
                o.size = (cui_control_size)size;
            else
                o.design_size = (cui_design_size)(size - 3);
            cui_button_options metrics = o;
            metrics.icon_size = 0;
            metrics.leading_artwork = NULL;
            metrics.icon = cui_string("check");
            scene(ui, &theme, metrics, 0);
            cui_rect expected_slot = cui_bounds(ui, 10 ^ UINT64_C(0x7185e28e9fb974db));
            metrics.leading_artwork = &art;
            metrics.icon = (cui_str){0};
            scene(ui, &theme, metrics, 0);
            cui_rect typed_slot = cui_bounds(ui, 10 ^ UINT64_C(0x7185e28e9fb974db));
            assert(fabsf(typed_slot.w - expected_slot.w) < .01f &&
                   fabsf(typed_slot.h - expected_slot.h) < .01f);
            const cui_draw_command *draw = image(scene(ui, &theme, o, 0));
            assert(draw && result.status == CUI_BUTTON_ARTWORK_OK);
            assert(draw->bitmap.bitmap.width == 24 && draw->bitmap.bitmap.height == 24);
            assert(draw->bitmap.tint.r == 231);
            cui_rect normal = cui_bounds(ui, 10);
            o.loading = true;
            draw = image(scene(ui, &theme, o, 0));
            assert(draw && draw->bitmap.tint.r != 231); /* ordinary Icon is replaced */
            assert(fabsf(cui_bounds(ui, 10).w - normal.w) < .01f);
            o.loading_artwork = &art;
            draw = image(scene(ui, &theme, o, 0));
            assert(draw && draw->bitmap.tint.r == 231);
            cui_icon_state saved_icon = icon;
            cui_button_artwork_result saved_result = result;
            cui_button_state state;
            memset(&state, 0x33, sizeof(state));
            cui_button_state saved_state = state;
            o.state = &state;
            assert(scene(ui, &theme, o, 2).count == 0);
            assert(!cui_needs_redraw(ui));
            assert(!memcmp(&icon, &saved_icon, sizeof(icon)));
            assert(!memcmp(&result, &saved_result, sizeof(result)));
            assert(!memcmp(&state, &saved_state, sizeof(state)));
            assert(fabsf(cui_bounds(ui, 10).w - normal.w) < .01f);
            o.leading_content = child;
            unsigned before = children;
            scene(ui, &theme, o, 0);
            assert(children == before + 1 && !memcmp(&result, &saved_result, sizeof(result)));
            for (unsigned vertical = 0; vertical < 2; vertical++) {
                cui_button_group_style_workspace workspace = {0};
                cui_button_group_styled_item items[2] = {
                    {.id = 20, .label = cui_string("One"), .options = {.leading_artwork = &art}},
                    {.id = 21, .label = cui_string("Two"), .options = {.leading_artwork = &art}}};
                assert(cui_begin_frame(ui, 2000, 1200, 0));
                cui_button_group_result group_result;
                assert(cui_button_group_ex(
                    ui, 30, &theme, items, selected, 2, &workspace,
                    (cui_button_group_style_options){
                        .override_size = true,
                        .size = o.size,
                        .design_size = o.design_size,
                        .behavior = {.vertical = vertical, .on_change = select_item}},
                    &group_result));
                cui_draw_list joined = cui_end_frame(ui);
                assert(image(joined));
                cui_rect a = cui_bounds(ui, 20), b = cui_bounds(ui, 21);
                assert(a.w > 0 && a.h > 0 && b.w > 0 && b.h > 0);
                assert(fabsf(vertical ? b.y - a.y - a.h : b.x - a.x - a.w) < .02f);
                assert(workspace.items[0].options.leading_artwork == &art);
                if (density == 1 && size == 17 && vertical == 1) {
                    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                           .x = b.x + b.w / 2,
                                                           .y = b.y + b.h / 2}));
                    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                           .x = b.x + b.w / 2,
                                                           .y = b.y + b.h / 2}));
                    assert(cui_begin_frame(ui, 2000, 1200, 0));
                    assert(cui_button_group_ex(
                        ui, 30, &theme, items, selected, 2, &workspace,
                        (cui_button_group_style_options){
                            .override_size = true,
                            .size = o.size,
                            .design_size = o.design_size,
                            .behavior = {.vertical = true, .on_change = select_item}},
                        &group_result));
                    assert(group_result.callbacks == 1 && !selected[0] && selected[1]);
                    assert(image(cui_end_frame(ui)));
                    assert(workspace.items[1].options.leading_artwork == &art);
                }
            }
            cui_icon_state_collect(&icon);
        }
    }
    cui_icon_options saved_options = art.icon_options;
    art.icon_options = (cui_icon_options){.custom_inherited_color = true,
                                          .inherited_color = {91, 160, 30, 255},
                                          .custom_opacity = true,
                                          .opacity = .4f};
    const cui_draw_command *custom =
        image(scene(ui, &theme, (cui_button_options){.leading_artwork = &art}, 0));
    assert(custom && custom->bitmap.tint.r == 91 && fabsf(custom->bitmap.opacity - .4f) < .01f);
    art.icon_options = saved_options;
    cui_button_options copied = {.leading_artwork = &art, .on_click = change_descriptor};
    mutable_art = &art;
    scene(ui, &theme, copied, 0);
    cui_rect hit = cui_bounds(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = hit.x + hit.w / 2,
                                           .y = hit.y + hit.h / 2}));
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_POINTER_UP, .x = hit.x + hit.w / 2, .y = hit.y + hit.h / 2}));
    assert(image(scene(ui, &theme, copied, 0)) && clicks == 1);
    assert(art.kind == CUI_BUTTON_ARTWORK_PROGRESS_CIRCLE); /* callback changed caller descriptor */
    art.kind = CUI_BUTTON_ARTWORK_ICON;
    unsigned char pixels[32 * 32 * 4];
    memset(pixels, 0x5a, sizeof(pixels));
    cui_progress_circle_state progress = {.pixels = pixels, .capacity = sizeof(pixels)};
    cui_button_artwork circle = {
        .kind = CUI_BUTTON_ARTWORK_PROGRESS_CIRCLE,
        .progress = &progress,
        .fraction = .75f,
        .result = &result,
        .progress_options = {.diameter = 300,
                             .raster_scale = 1,
                             .progress = {.custom_color = true, .color = {10, 180, 30, 255}}}};
    cui_button_options o = {
        .leading_artwork = &circle, .loading_artwork = &art, .loading = true, .icon_size = 24};
    const cui_draw_command *draw = image(scene(ui, &theme, o, 0));
    assert(draw && draw->bitmap.bitmap.width == 24 && progress.revision &&
           result.status == CUI_BUTTON_ARTWORK_OK);
    unsigned char expected[sizeof(pixels)];
    memcpy(expected, pixels, sizeof(pixels));
    o.loading = false;
    scene(ui, &theme, o, 0);
    assert(!memcmp(expected, pixels, 24 * 24 * 4)); /* typed Progress survives loading */
    cui_button_artwork loading_copy = art;
    for (unsigned density = 0; density < 2; density++) {
        theme.font_size = density ? 14 : 16;
        for (unsigned size = 0; size < 18; size++) {
            o = (cui_button_options){.leading_artwork = &circle,
                                     .loading_artwork = &loading_copy,
                                     .loading = true,
                                     .icon_size = 24};
            if (size < 4)
                o.size = (cui_control_size)size;
            else
                o.design_size = (cui_design_size)(size - 3);
            draw = image(scene(ui, &theme, o, 0));
            assert(draw && draw->bitmap.bitmap.width == 24 && !draw->bitmap.custom_tint);
            o.leading_artwork = &art;
            art.kind = CUI_BUTTON_ARTWORK_SPINNER;
            draw = image(scene(ui, &theme, o, 0));
            assert(draw && draw->bitmap.bitmap.width > 0 && draw->bitmap.tint.r == 231);
            assert(fabsf(cui_bounds(ui, 10 ^ UINT64_C(0x7185e28e9fb974db)).w - 24) < .01f);
            cui_icon_state_collect(&icon);
        }
    }
    art.kind = CUI_BUTTON_ARTWORK_ICON;
    o = (cui_button_options){.leading_artwork = &circle, .icon_size = 24};
    progress.capacity = 1;
    uint64_t revision = progress.revision;
    assert(!image(scene(ui, &theme, o, 0)));
    assert(result.status == CUI_BUTTON_ARTWORK_PROGRESS_FAILURE && progress.revision == revision);
    assert(!memcmp(expected, pixels, sizeof(pixels)));
    cui_icon_state limited;
    cui_icon_state_init(&limited);
    assert(cui_icon_set_data(
        &limited, cui_string("<svg viewBox='0 0 16 16'><path d='M0 0h16v16H0z'/></svg>")));
    assert(cui_icon_set_limits(&limited, (cui_icon_limits){.max_pixels = 1}));
    art.icon = &limited;
    o = (cui_button_options){.leading_artwork = &art, .icon_size = 24};
    assert(!image(scene(ui, &theme, o, 0)));
    assert(result.status == CUI_BUTTON_ARTWORK_ICON_FAILURE && result.icon_error == CUI_ICON_LIMIT);
    cui_icon_state_destroy(&limited);
    art.icon = &icon;
    art.kind = CUI_BUTTON_ARTWORK_SPINNER;
    o.loading = true;
    draw = image(scene(ui, &theme, o, 0));
    assert(draw && draw->bitmap.tint.r == 231); /* explicit Spinner retained */
    cui_set_reduced_motion(ui, true);
    scene(ui, &theme, o, 0);
    assert(!cui_needs_redraw(ui));
    cui_set_reduced_motion(ui, false);
    scene(ui, &theme, o, 1);
    assert(cui_needs_redraw(ui)); /* visible inert Spinner still animates */
    cui_icon_state saved = icon;
    scene(ui, &theme, o, 2);
    assert(!cui_needs_redraw(ui) && !memcmp(&saved, &icon, sizeof(saved)));
    art.fraction = NAN; /* irrelevant to an Icon/Spinner descriptor */
    scene(ui, &theme, o, 0);
    cui_button_artwork bad = circle;
    bad.fraction = NAN;
    o.leading_artwork = &bad;
    assert(scene(ui, &theme, o, 0).count == 0);
    o.leading_artwork = &art;
    cui_button_artwork_result *saved_output = art.result;
    art.result = (cui_button_artwork_result *)(void *)&art;
    assert(scene(ui, &theme, o, 0).count == 0);
    art.result = saved_output;
    o.loading_artwork = &circle; /* only Icon accepted for loading replacement */
    assert(scene(ui, &theme, o, 0).count == 0);
    cui_icon_state_destroy(&icon);
    cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 128,
                                 .max_commands = 512,
                                 .max_resources = 1,
                                 .resource_bytes = 256,
                                 .frame_bytes = 65536});
    assert(ui && cui_begin_frame(ui, 800, 600, 0));
    assert(cui_resource_get(ui, 100, 100, 1, NULL, NULL));
    cui_button_artwork default_spinner = {.kind = CUI_BUTTON_ARTWORK_SPINNER, .result = &result};
    (void)cui_button_ex(ui, 10, &theme, cui_string("Resource limit"),
                        (cui_button_options){.leading_artwork = &default_spinner, .icon_size = 24});
    assert(!image(cui_end_frame(ui)));
    assert(result.status == CUI_BUTTON_ARTWORK_ICON_FAILURE &&
           result.icon_error == CUI_ICON_ALLOCATION);
    assert(fabsf(cui_bounds(ui, 10 ^ UINT64_C(0x7185e28e9fb974db)).w - 24) < .01f);
    cui_destroy(ui);
    return 0;
}
