/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
static cui_context *ui;
static cui_theme theme;
static cui_checkbox_state state;
static cui_check_state value;
static cui_checkbox_options options;
static unsigned changes;
static void detail(cui_context *context, void *user) {
    (void)user;
    cui_text(context, cui_string("Secondary styled notification details"), 14, theme.muted);
}
static cui_draw_list frame(void) {
    assert(cui_begin_frame(ui, 300, 12000, .016f));
    changes += cui_checkbox_ex(ui, 11, &theme,
                               cui_string("A long wrapped notification label for this account"),
                               &value, options);
    return cui_end_frame(ui);
}
int main(void) {
    ui = cui_create((cui_config){.width = 300, .height = 300});
    assert(ui);
    theme = cui_theme_light();
    options = (cui_checkbox_options){
        .state = &state,
        .width = 120,
        .content = detail,
        .tooltip = {"Notification help", 17},
        .tooltip_options = {.viewport = {0, 0, 300, 300}, .open_delay = .05f}};
    frame();
    assert(state.checked.progress == 0 && state.next_wake_seconds == 0);
    value = CUI_CHECK_CHECKED;
    cui_draw_list animated = frame();
    size_t semantics_count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &semantics_count);
    assert(semantics_count && semantics[0].checked && semantics[0].value == CUI_CHECK_CHECKED);
    bool partial = false;
    for (size_t i = 0; i < animated.count; i++)
        if (animated.commands[i].type == CUI_DRAW_IMAGE && animated.commands[i].bitmap.tint.a > 0 &&
            animated.commands[i].bitmap.tint.a < theme.on_primary.a)
            partial = true;
    assert(partial && changes == 0);
    theme.on_primary = (cui_color){251, 13, 77, 255};
    animated = frame();
    bool recolored = false;
    for (size_t i = 0; i < animated.count; i++)
        if (animated.commands[i].type == CUI_DRAW_IMAGE)
            recolored = animated.commands[i].bitmap.tint.r == 251 &&
                        animated.commands[i].bitmap.tint.b == 77;
    assert(recolored);
    theme.on_primary = cui_theme_light().on_primary;
    assert(state.checked.progress > 0 && state.checked.progress < 1);
    assert(state.next_wake_seconds > cui_frame_time(ui));
    assert(cui_needs_redraw(ui));
    value = CUI_CHECK_MIXED;
    frame();
    assert(state.checked.progress > 0 && state.mixed.progress > 0);
    value = CUI_CHECK_UNCHECKED;
    for (unsigned i = 0; i < 160; i++)
        frame();
    assert(state.checked.progress == 0 && state.mixed.progress == 0 &&
           state.next_wake_seconds == 0 && changes == 0);
    options.reduced_motion = true;
    value = CUI_CHECK_CHECKED;
    frame();
    assert(state.checked.progress == 1 && state.checked.velocity == 0 &&
           state.next_wake_seconds == 0);
    options.reduced_motion = false;
    theme.spring_control.response = 0;
    value = CUI_CHECK_MIXED;
    frame();
    assert(state.mixed.progress == 1 && state.checked.progress == 0);
    theme = cui_theme_light();
    cui_rect bounds = cui_bounds(ui, 11);
    assert(bounds.w >= 119 && bounds.h > theme.font_size * 2);
    cui_focus(ui, 11);
    frame();
    assert(state.tooltip.open);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE});
    frame();
    assert(!state.tooltip.open);
    options.disabled = true;
    frame();
    assert(!state.tooltip.open && !state.tooltip.pending);
    cui_check_state before = value;
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE});
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE});
    frame();
    assert(value == before);
    options.disabled = false;
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE});
    frame();
    cui_focus(ui, 11);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE});
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE});
    frame();
    assert(value != before);
    frame();
    bounds = cui_bounds(ui, 11);
    before = value;
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                    .pointer_type = CUI_POINTER_TOUCH,
                                    .pointer_id = 71,
                                    .x = bounds.x + 5,
                                    .y = bounds.y + 5});
    frame();
    cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_CANCEL, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 71});
    frame();
    assert(value == before && !state.tooltip.open);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                    .pointer_type = CUI_POINTER_TOUCH,
                                    .pointer_id = 71,
                                    .x = bounds.x + 5,
                                    .y = bounds.y + 5});
    frame();
    assert(value == before);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                    .pointer_type = CUI_POINTER_TOUCH,
                                    .pointer_id = 72,
                                    .x = bounds.x + 5,
                                    .y = bounds.y + 5});
    frame();
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                    .pointer_type = CUI_POINTER_TOUCH,
                                    .pointer_id = 72,
                                    .x = bounds.x + 5,
                                    .y = bounds.y + 5});
    frame();
    assert(value != before && changes == 2);
    for (unsigned density = 0; density < 2; density++) {
        theme.font_size = density ? 14 : 16;
        for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; size++) {
            options.design_size = (cui_design_size)size;
            cui_design_metrics metrics;
            assert(cui_design_measure((cui_design_size)size, theme.font_size / 16, &metrics));
            options.width = fmaxf(200, metrics.indicator_size + 8 * theme.font_size / 16 +
                                           14 * metrics.font_size);
            options.tooltip = (cui_str){0};
            cui_draw_list d = frame();
            bounds = cui_bounds(ui, 11);
            for (size_t i = 0; i < d.count; i++)
                if (d.commands[i].type == CUI_DRAW_TEXT) {
                    assert(d.commands[i].bounds.x >= bounds.x - .01f);
                    assert(d.commands[i].bounds.x + d.commands[i].bounds.w <=
                           bounds.x + bounds.w + .01f);
                    assert(d.commands[i].bounds.y >= bounds.y - .01f);
                    assert(d.commands[i].bounds.y + d.commands[i].bounds.h <=
                           bounds.y + bounds.h + .01f);
                }
        }
    }
    theme.font_size = 16;
    options.design_size = CUI_DESIGN_INHERIT;
    options.content = NULL;
    options.width = 120;
    options.tooltip = cui_string("Hover help");
    options.tooltip_options.open_delay = .2f;
    cui_focus(ui, 0);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 290, .y = 290});
    frame();
    bounds = cui_bounds(ui, 11);
    cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 5, .y = bounds.y + 5});
    frame();
    assert(state.tooltip.pending && !state.tooltip.open);
    for (unsigned i = 0; i < 20; i++)
        frame();
    assert(state.tooltip.open);
    options.disabled = true;
    frame();
    assert(!state.tooltip.open && !state.tooltip.pending);
    options.disabled = false;
    options.tooltip = (cui_str){0};
    for (unsigned policy = 0; policy < 2; policy++) {
        theme.spring_control.damping = policy ? 2 : .4f;
        value = CUI_CHECK_CHECKED;
        for (unsigned i = 0; i < 400; i++)
            frame();
        assert(state.checked.progress == 1 && state.checked.velocity == 0);
        value = CUI_CHECK_UNCHECKED;
        for (unsigned i = 0; i < 400; i++)
            frame();
        assert(state.checked.progress == 0 && state.checked.velocity == 0 &&
               state.next_wake_seconds == 0);
    }
    unsigned final_changes = changes;
    assert(final_changes == 2);
    theme.spring_control = (cui_spring_settings){FLT_MIN, 0, .001f, true};
    value = CUI_CHECK_CHECKED;
    frame();
    assert(isfinite(state.checked.progress) && isfinite(state.checked.velocity));
    theme = cui_theme_light();
    value = CUI_CHECK_CHECKED;
    assert(cui_begin_frame(ui, 300, 12000, .016f));
    assert(!cui_checkbox_ex(ui, 12, &theme, cui_string("Replacement"), &value, options));
    cui_end_frame(ui);
    assert(state.id == 12 && state.checked.progress == 1 && state.mixed.progress == 0 &&
           state.next_wake_seconds == 0 && !state.tooltip.open);
    cui_destroy(ui);
    puts("checkbox motion: ok");
}
