/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned callbacks;
static void content(cui_context *ui, void *user) {
    (void)ui;
    (void)user;
    ++callbacks;
}
static cui_draw_list scene(cui_context *ui, cui_theme *theme, cui_choice_item items[2],
                           cui_id *selected, cui_tabs_options options, float opacity) {
    assert(cui_begin_frame(ui, 320, 180, .016f));
    cui_begin_box(ui, 777,
                  (cui_box){.width = 320,
                            .height = 180,
                            .padding = 20,
                            .custom_opacity = true,
                            .opacity = opacity});
    (void)cui_tabs(ui, 101, theme, items, 2, selected, options, NULL, NULL);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static size_t layers(cui_draw_list list, float first, float second, unsigned alpha,
                     cui_rect *caster, float radius) {
    size_t count = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type != CUI_DRAW_DROP_SHADOW)
            continue;
        assert(count < 2);
        assert(c->shadow.layer.offset_y == 1 && c->shadow.layer.offset_x == 0);
        assert(c->shadow.layer.sigma == (count ? second : first));
        assert(c->shadow.layer.spread == (count ? -1 : 0));
        assert(c->shadow.layer.color.a == alpha && c->shadow.layer.color.r == 0);
        assert(c->shadow.radii.top_left == radius);
        if (!count)
            *caster = c->shadow.caster;
        else
            assert(c->shadow.caster.x == caster->x && c->shadow.caster.y == caster->y &&
                   c->shadow.caster.w == caster->w && c->shadow.caster.h == caster->h);
        ++count;
    }
    return count;
}
int main(void) {
    for (unsigned dark = 0; dark < 2; ++dark)
        for (unsigned size = 0; size < 4; ++size) {
            cui_context *ui = cui_create((cui_config){.width = 320, .height = 180});
            assert(ui);
            cui_theme theme = dark ? cui_theme_dark() : cui_theme_light();
            cui_choice_item items[] = {{10, {"A", 1}, false}, {20, {"B", 1}, false}};
            cui_id selected = 10;
            cui_tabs_options options = {.variant = CUI_TAB_SEGMENTED,
                                        .size = (cui_tab_size)size,
                                        .density = 1,
                                        .shadow_policy = CUI_TAB_SHADOW_SOURCE_TAB};
            cui_rect caster = {0};
            float radius =
                fmaxf(0, (size == CUI_TAB_MEDIUM || size == CUI_TAB_LARGE ? theme.radius_lg
                                                                          : theme.radius) -
                             (size == CUI_TAB_LARGE ? 3 : 2));
            cui_draw_list list = scene(ui, &theme, items, &selected, options, 1);
            assert(layers(list, 2, 0, 13, &caster, radius) == 1);
            cui_rect item = cui_bounds(ui, 10);
            assert(caster.w == item.w &&
                   caster.h ==
                       cui_tab_size_metrics(CUI_TAB_SEGMENTED, (cui_tab_size)size, 1).inner_height);
            list = scene(ui, &theme, items, &selected, options, .5f);
            assert(layers(list, 2, 0, 6, &caster, radius) == 1);
            items[0].disabled = true;
            list = scene(ui, &theme, items, &selected, options, 1);
            assert(layers(list, 0, 0, 0, &caster, radius) == 0 && selected == 10);
            items[0].disabled = false;
            cui_tab_animation animation = {0};
            options.animation = &animation;
            options.shadow_policy = CUI_TAB_SHADOW_SOURCE_BAR;
            /* A new container makes its first measure genuinely unavailable. */
            cui_context *first = cui_create((cui_config){.width = 320, .height = 180});
            assert(first);
            list = scene(first, &theme, items, &selected, options, 1);
            assert(!animation.ready && layers(list, 3, 2, 26, &caster, radius) == 2);
            list = scene(first, &theme, items, &selected, options, 1);
            assert(animation.ready && layers(list, 1.5f, 1, 26, &caster, radius) == 2);
            item = cui_bounds(first, 10);
            assert(caster.x == item.x && caster.w == item.w);
            list = scene(first, &theme, items, &selected, options, .5f);
            assert(layers(list, 1.5f, 1, 13, &caster, radius) == 2);
            items[0].disabled = true;
            list = scene(first, &theme, items, &selected, options, 1);
            assert(animation.ready && layers(list, 1.5f, 1, 26, &caster, radius) == 2 &&
                   selected == 10);
            assert(cui_queue_event(first,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            list = scene(first, &theme, items, &selected, options, 1);
            assert(selected == 10 && layers(list, 1.5f, 1, 26, &caster, radius) == 2);
            /* Externally controlled replacement moves the actual caster. */
            selected = 20;
            options.reduced_motion = true;
            list = scene(first, &theme, items, &selected, options, 1);
            assert(layers(list, 1.5f, 1, 26, &caster, radius) == 2);
            assert(caster.x == cui_bounds(first, 20).x && caster.w == cui_bounds(first, 20).w);
            item = cui_bounds(first, 10);
            assert(cui_queue_event(first, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                      .x = item.x + item.w * .5f,
                                                      .y = item.y + item.h * .5f}));
            assert(cui_queue_event(first, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                      .x = item.x + item.w * .5f,
                                                      .y = item.y + item.h * .5f}));
            (void)scene(first, &theme, items, &selected, options, 1);
            assert(selected == 20); /* Disabled tab cannot steal controlled selection. */
            items[0].disabled = false;
            (void)scene(first, &theme, items, &selected, options, 1);
            item = cui_bounds(first, 10);
            assert(cui_queue_event(first, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                      .x = item.x + item.w * .5f,
                                                      .y = item.y + item.h * .5f}));
            assert(cui_queue_event(first, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                      .x = item.x + item.w * .5f,
                                                      .y = item.y + item.h * .5f}));
            list = scene(first, &theme, items, &selected, options, 1);
            assert(selected == 10 && layers(list, 1.5f, 1, 26, &caster, radius) == 2);
            /* Frame payload has no borrowed pointer into mutable spring state. */
            assert(cui_begin_frame(first, 320, 180, .016f));
            cui_begin_box(first, 777, (cui_box){.width = 320, .height = 180, .padding = 20});
            (void)cui_tabs(first, 101, &theme, items, 2, &selected, options, NULL, NULL);
            float prepared_width = animation.spring.width;
            animation.spring.width = 999;
            cui_end_box(first);
            list = cui_end_frame(first);
            assert(layers(list, 1.5f, 1, 26, &caster, radius) == 2 && caster.w == prepared_width);
            animation.spring.width = prepared_width;
            options.variant = CUI_TAB_PILL;
            list = scene(first, &theme, items, &selected, options, 1);
            assert(layers(list, 0, 0, 0, &caster, radius) == 0);
            options.variant = CUI_TAB_SEGMENTED;
            options.shadow_policy = CUI_TAB_SHADOW_LEGACY;
            list = scene(first, &theme, items, &selected, options, 1);
            assert(layers(list, 0, 0, 0, &caster, radius) == 0);
            cui_destroy(first);
            cui_destroy(ui);
        }
    {
        cui_context *scroll_ui = cui_create((cui_config){.width = 320, .height = 180});
        assert(scroll_ui);
        cui_theme theme = cui_theme_light();
        cui_choice_item items[] = {{10, {"A", 1}, false}, {20, {"B", 1}, false}};
        cui_id selected = 10;
        cui_tab_animation animation = {0};
        cui_tab_scroll_state scroll = {0};
        cui_tabs_options options = {.variant = CUI_TAB_SEGMENTED,
                                    .density = 1,
                                    .shadow_policy = CUI_TAB_SHADOW_SOURCE_BAR,
                                    .animation = &animation,
                                    .scroll = &scroll,
                                    .bar_extent = 40,
                                    .reduced_motion = true};
        cui_rect caster = {0};
        for (unsigned frame = 0; frame < 3; ++frame)
            (void)scene(scroll_ui, &theme, items, &selected, options, 1);
        cui_draw_list list = scene(scroll_ui, &theme, items, &selected, options, 1);
        assert(layers(list, 1.5f, 1, 26, &caster, theme.radius_lg - 2) == 2 && scroll.maximum >= 8);
        float before_x = caster.x;
        scroll.offset = 8;
        list = scene(scroll_ui, &theme, items, &selected, options, 1);
        assert(layers(list, 1.5f, 1, 26, &caster, theme.radius_lg - 2) == 2 &&
               caster.x == before_x - 8);
        list = scene(scroll_ui, &theme, items, &selected, options, 1);
        assert(layers(list, 1.5f, 1, 26, &caster, theme.radius_lg - 2) == 2 &&
               caster.x == before_x - 8);
        cui_destroy(scroll_ui);
    }
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_choice_item items[] = {{10, {"A", 1}, false}, {20, {"B", 1}, false}};
    cui_id selected = 10;
    cui_tab_drag_state drag = {.requested = true, .candidate = 20, .active = true};
    cui_tab_animation animation = {.selected = 99, .ready = true};
    cui_tab_scroll_state scroll = {.offset = 2, .changed = true};
    cui_tabs_options options = {.variant = CUI_TAB_SEGMENTED,
                                .shadow_policy = (cui_tab_shadow_policy)99,
                                .animation = &animation,
                                .drag = &drag,
                                .scroll = &scroll,
                                .prefix = content};
    cui_tab_drag_state before_drag;
    memcpy(&before_drag, &drag, sizeof(drag));
    cui_tab_animation before_animation;
    memcpy(&before_animation, &animation, sizeof(animation));
    cui_tab_scroll_state before_scroll;
    memcpy(&before_scroll, &scroll, sizeof(scroll));
    assert(cui_begin_frame(ui, 320, 180, .016f));
    assert(!cui_tabs(ui, 101, &theme, items, 2, &selected, options, NULL, NULL));
    assert(!cui_base_tabs(ui, items, 2, &selected, options));
    assert(!memcmp(&drag, &before_drag, sizeof(drag)) &&
           !memcmp(&animation, &before_animation, sizeof(animation)) &&
           !memcmp(&scroll, &before_scroll, sizeof(scroll)) && callbacks == 0 && selected == 10);
    options.shadow_policy = CUI_TAB_SHADOW_SOURCE_BAR;
    options.animation = NULL;
    assert(!cui_tabs(ui, 101, &theme, items, 2, &selected, options, NULL, NULL));
    options.animation = &animation;
    options.vertical = true;
    assert(!cui_tabs(ui, 101, &theme, items, 2, &selected, options, NULL, NULL));
    (void)cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
