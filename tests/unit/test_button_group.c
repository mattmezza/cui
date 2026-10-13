/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
static unsigned calls;
static unsigned slot_calls, hover_calls;
static cui_button_group_style_workspace *reentry_workspace;
static const cui_theme *reentry_theme;
static unsigned reentry_calls;
static void reentry(cui_context *ui, void *user) {
    (void)user;
    ++reentry_calls;
    assert(!cui_button_group_ex(ui, 500, reentry_theme, NULL, NULL, 0, reentry_workspace,
                                (cui_button_group_style_options){0}, NULL));
}
static void slot(cui_context *ui, void *user) {
    (void)user;
    ++slot_calls;
    cui_text(ui, cui_string("Slot"), 14, (cui_color){255, 255, 255, 255});
}
static void hover(void *user, bool hovered) {
    (void)user;
    (void)hovered;
    ++hover_calls;
}
static void click(void *user) {
    ++*(unsigned *)user;
}
static bool values[3];
static void change(void *user, const cui_button_group_request *r) {
    (void)user;
    ++calls;
    for (size_t i = 0; i < 3; i++)
        values[i] = false;
    for (size_t i = 0; i < r->selected_count; i++)
        values[r->selected_indices[i]] = true;
}
static cui_draw_list scene(cui_context *ui, cui_button_group_style_workspace *w,
                           const cui_theme *theme, cui_button_group_style_options options,
                           cui_button_group_styled_item *items) {
    assert(cui_begin_frame(ui, 8000, 8000, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 8000, .height = 8000, .padding = 10});
    cui_button_group_result result;
    assert(cui_button_group_ex(ui, 100, theme, items, values, 3, w, options, &result));
    assert(result.valid);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
static const cui_draw_command *border(cui_draw_list d, cui_rect bounds) {
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].bounds.x == bounds.x && d.commands[i].bounds.y == bounds.y &&
            d.commands[i].bounds.w == bounds.w && d.commands[i].bounds.h == bounds.h &&
            d.commands[i].type == CUI_DRAW_BORDER)
            return &d.commands[i];
    return NULL;
}
static void pointer(cui_context *ui, cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    float x = b.x + b.w / 2, y = b.y + b.h / 2;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
}
int main(void) {
    cui_button_group_style_workspace *w = calloc(1, sizeof *w);
    assert(w);
    cui_button_group_styled_item items[3] = {{.id = 10, .label = {"One", 3}},
                                             {.id = 11, .label = {"Two", 3}},
                                             {.id = 12, .label = {"Three", 5}}};
    for (unsigned size = 0; size < 18; size++)
        for (unsigned compact = 0; compact < 2; compact++)
            for (unsigned vertical = 0; vertical < 2; vertical++)
                for (unsigned variant = 0; variant <= CUI_BUTTON_CUSTOM; variant++) {
                    values[0] = values[2] = true;
                    values[1] = false;
                    cui_context *ui = cui_create((cui_config){0});
                    assert(ui);
                    cui_theme theme = cui_theme_light();
                    if (compact)
                        theme.font_size = 14;
                    cui_button_group_style_options o = {
                        .behavior = {.vertical = vertical},
                        .override_size = true,
                        .size = (cui_control_size)(size < 4 ? size : 0),
                        .override_variant = true,
                        .variant = (cui_button_variant)variant,
                        .design_size = size < 4 ? CUI_DESIGN_INHERIT : (cui_design_size)(size - 3)};
                    cui_draw_list d = scene(ui, w, &theme, o, items);
                    cui_rect boxes[3];
                    for (size_t i = 0; i < 3; i++) {
                        boxes[i] = cui_bounds(ui, 10 + i);
                        assert(boxes[i].w > 0 && boxes[i].h > 0);
                    }
                    for (size_t i = 1; i < 3; i++) {
                        assert(fabsf((vertical ? boxes[i].y : boxes[i].x) -
                                     (vertical ? boxes[i - 1].y + boxes[i - 1].h
                                               : boxes[i - 1].x + boxes[i - 1].w)) < .01f);
                    }
                    if (variant != CUI_BUTTON_GHOST && variant != CUI_BUTTON_LINK &&
                        variant != CUI_BUTTON_TEXT) {
                        for (size_t i = 0; i < 3; i++) {
                            const cui_draw_command *b = border(d, boxes[i]);
                            assert(b && b->custom_corners && b->custom_border_edges);
                            assert(b->border_edges.left == (vertical || i == 0 ? 1 : 0));
                            assert(b->border_edges.top == (!vertical || i == 0 ? 1 : 0));
                            assert(b->border_edges.right == 1 && b->border_edges.bottom == 1);
                            assert((b->corner_radii.top_left > 0) == (i == 0));
                            assert((b->corner_radii.bottom_right > 0) == (i == 2));
                            assert((b->corner_radii.top_right > 0) == (vertical ? i == 0 : i == 2));
                            assert((b->corner_radii.bottom_left > 0) ==
                                   (vertical ? i == 2 : i == 0));
                        }
                    }
                    for (size_t i = 0; i < d.count; i++)
                        if (d.commands[i].type == CUI_DRAW_TEXT) {
                            cui_rect b = d.commands[i].bounds;
                            bool contained = false;
                            for (size_t j = 0; j < 3; j++)
                                contained |= b.x >= boxes[j].x - .01f && b.y >= boxes[j].y - .01f &&
                                             b.x + b.w <= boxes[j].x + boxes[j].w + .01f &&
                                             b.y + b.h <= boxes[j].y + boxes[j].h + .01f;
                            assert(contained);
                        }
                    assert(cui_last_error(ui) == CUI_OK);
                    cui_destroy(ui);
                }
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_button_group_style_options o = {.behavior = {.on_change = change}};
    values[0] = values[2] = false;
    values[1] = true;
    items[0].options = (cui_button_options){.size = CUI_CONTROL_XSMALL,
                                            .icon = {"check", 5},
                                            .custom_cursor = true,
                                            .cursor = CUI_CURSOR_CROSSHAIR};
    items[1].options = (cui_button_options){.size = CUI_CONTROL_LARGE, .loading = true};
    items[2].options = (cui_button_options){.disabled = true};
    scene(ui, w, &theme, o, items);
    scene(ui, w, &theme, o, items);
    assert(cui_bounds(ui, 10).h < cui_bounds(ui, 11).h);
    calls = 0;
    pointer(ui, 11);
    pointer(ui, 12);
    scene(ui, w, &theme, o, items);
    assert(!calls);
    pointer(ui, 10);
    pointer(ui, 10);
    cui_draw_list selected_draw = scene(ui, w, &theme, o, items);
    assert(calls == 2 && values[0]);
    cui_rect first_bounds = cui_bounds(ui, 10);
    bool active_paint = false;
    for (size_t i = 0; i < selected_draw.count; i++) {
        const cui_draw_command *c = &selected_draw.commands[i];
        if (c->type == CUI_DRAW_RECT && c->bounds.x == first_bounds.x &&
            c->bounds.y == first_bounds.y)
            active_paint |= c->color.r == theme.buttons[CUI_THEME_BUTTON_PRIMARY].active.r &&
                            c->color.g == theme.buttons[CUI_THEME_BUTTON_PRIMARY].active.g &&
                            c->color.b == theme.buttons[CUI_THEME_BUTTON_PRIMARY].active.b;
    }
    assert(active_paint); /* Already-registered first sibling paints final accepted model. */
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_CROSSHAIR);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 10)
            assert(sem[i].checked && sem[i].value == 1);
    /* Standalone Button callback multiplicity retains its any-activation bool. */
    assert(cui_begin_frame(ui, 300, 100, 0));
    cui_button_ex(ui, 50, &theme, cui_string("Standalone"),
                  (cui_button_options){.on_click = click, .action_user = &calls});
    cui_end_frame(ui);
    calls = 0;
    pointer(ui, 50);
    pointer(ui, 50);
    assert(cui_begin_frame(ui, 300, 100, 0));
    assert(cui_button_ex(ui, 50, &theme, cui_string("Standalone"),
                         (cui_button_options){.on_click = click, .action_user = &calls}));
    assert(calls == 2);
    cui_end_frame(ui);
    cui_destroy(ui);
    /* Styled passive layout/paint participates without semantics or lifecycle callbacks. */
    ui = cui_create((cui_config){0});
    assert(ui);
    cui_button_state states[3] = {{0}};
    for (size_t i = 0; i < 3; i++)
        items[i].options = (cui_button_options){.state = &states[i],
                                                .on_hover = hover,
                                                .on_click = click,
                                                .action_user = &calls,
                                                .rounding = CUI_BUTTON_ROUNDED_CUSTOM,
                                                .radius = 0};
    items[2].options.leading_content = slot;
    items[2].options.children = slot;
    o = (cui_button_group_style_options){.behavior = {.on_change = change}};
    scene(ui, w, &theme, o, items);
    cui_rect expected[3];
    for (size_t i = 0; i < 3; i++)
        expected[i] = cui_bounds(ui, 10 + i);
    for (unsigned ancestor = 0; ancestor < 2; ancestor++)
        for (unsigned layout = 0; layout < 2; layout++) {
            for (size_t i = 0; i < 3; i++)
                states[i] = (cui_button_state){0};
            calls = hover_calls = slot_calls = 0;
            bool before[3] = {values[0], values[1], values[2]};
            assert(cui_begin_frame(ui, 8000, 8000, 0));
            cui_begin_box(ui, 1,
                          (cui_box){.width = 8000,
                                    .height = 8000,
                                    .padding = 10,
                                    .inert = ancestor && !layout,
                                    .layout_only = ancestor && layout});
            assert(cui_frame_is_submitting(ui));
            o.box = (cui_box){.inert = !ancestor && !layout, .layout_only = !ancestor && layout};
            cui_button_group_result r;
            assert(cui_button_group_ex(ui, 100, &theme, items, values, 3, w, o, &r));
            assert(r.valid && !r.callbacks && !r.accepted_activations);
            cui_end_box(ui);
            cui_draw_list passive_draw = cui_end_frame(ui);
            size_t semantic_count;
            cui_semantics(ui, &semantic_count);
            assert(!semantic_count);
            assert(!calls && !hover_calls && slot_calls == 2);
            for (size_t i = 0; i < 3; i++) {
                cui_rect b = cui_bounds(ui, 10 + i);
                assert(b.w == expected[i].w && b.h == expected[i].h);
                assert(!states[i].initialized && values[i] == before[i]);
            }
            assert(layout ? !passive_draw.count : passive_draw.count > 0);
        }
    o.box = (cui_box){0};
    scene(ui, w, &theme, o, items);
    for (size_t i = 0; i < 3; i++)
        assert(states[i].initialized);
    pointer(ui, 10);
    calls = 0;
    scene(ui, w, &theme, o, items);
    assert(calls == 1);
    /* Custom paint callbacks cannot overwrite an in-progress descriptor snapshot. */
    reentry_workspace = w;
    reentry_theme = &theme;
    items[0].options.children = reentry;
    items[0].options.children_user = NULL;
    scene(ui, w, &theme, o, items);
    assert(reentry_calls == 1 && !w->composing);
    items[0].options.children = NULL;
    /* One child retains explicit zero rounding instead of inheriting group corners. */
    assert(cui_begin_frame(ui, 300, 100, 0));
    cui_button_group_result single;
    assert(cui_button_group_ex(ui, 100, &theme, items, values, 1, w, o, &single));
    cui_draw_list single_draw = cui_end_frame(ui);
    const cui_draw_command *single_border = border(single_draw, cui_bounds(ui, 10));
    assert(single_border && single_border->radius == 0 &&
           single_border->corner_radii.top_left == 0 &&
           single_border->corner_radii.top_right == 0 &&
           single_border->corner_radii.bottom_left == 0 &&
           single_border->corner_radii.bottom_right == 0);
    cui_destroy(ui);
    free(w);
}
