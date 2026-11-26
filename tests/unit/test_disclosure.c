/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_disclosure_events.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_disclosure_item items[] = {
    {10, {"One", 3}, false}, {20, {"Two", 3}, true}, {30, {"Three", 5}, false}};
static bool opened[3];
static unsigned panels;
static void panel(cui_context *ui, size_t index, void *user) {
    (void)user;
    panels |= 1u << index;
    cui_button(ui, 100 + index, &(cui_theme){.font_size = 14, .control_height = 32},
               cui_string("Inside"), false);
}
static bool scene(cui_context *ui, cui_accordion_options options, const cui_theme *theme) {
    panels = 0;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_accordion(ui, 1, theme, items, 3, opened, options, panel, NULL);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count > 0 && cui_last_error(ui) == CUI_OK);
    return changed;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void click(cui_context *ui, cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
}
static unsigned headers, expanded_headers, disabled_headers;
static void custom_header(cui_context *ui, const cui_disclosure_item *item, size_t index,
                          bool expanded, void *user) {
    assert(user == &headers);
    headers |= 1u << index;
    if (expanded)
        expanded_headers |= 1u << index;
    if (item->disabled)
        disabled_headers |= 1u << index;
    cui_text(ui, cui_string(expanded ? "[open]" : "[closed]"), 12, (cui_color){1, 2, 3, 255});
    cui_text(ui, cui_string("Custom title"), 12, (cui_color){4, 5, 6, 255});
}
static void test_custom_disclosure_headers(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    for (size_t i = 0; i < 3; i++)
        opened[i] = false;
    cui_accordion_options options = {
        .collapsible = true, .render_header = custom_header, .header_user = &headers};
    headers = expanded_headers = disabled_headers = 0;
    scene(ui, options, &theme);
    assert(headers == 7 && expanded_headers == 0 && disabled_headers == 2);
    click(ui, 10);
    assert(scene(ui, options, &theme));
    assert(expanded_headers == 1 && opened[0] && panels == 1);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    bool named = false;
    for (size_t i = 0; i < count; i++)
        named |=
            semantics[i].id == 10 && semantics[i].name.length == 3 && semantics[i].details.expanded;
    assert(named);
    options.disabled = true;
    disabled_headers = 0;
    scene(ui, options, &theme);
    assert(disabled_headers == 7);
    bool open = false;
    cui_collapsible_options collapse = {
        .borderless = true, .render_header = custom_header, .header_user = &headers};
    headers = expanded_headers = 0;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_collapsible_ex(ui, 200, &theme, cui_string("Accessible title"), &open, collapse, panel,
                       NULL);
    cui_end_frame(ui);
    assert(headers == 1 && !expanded_headers);
    click(ui, 200);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible_ex(ui, 200, &theme, cui_string("Accessible title"), &open, collapse,
                              panel, NULL));
    cui_end_frame(ui);
    assert(open && expanded_headers == 1);
    cui_focus(ui, 200);
    key(ui, CUI_KEY_ENTER);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible_ex(ui, 200, &theme, cui_string("Accessible title"), &open, collapse,
                              panel, NULL));
    cui_end_frame(ui);
    assert(!open);
    cui_destroy(ui);
}
static void test_disclosure_box_overrides(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    const cui_box trigger[] = {
        {.width = 200, .height = 56, .padding = 2, .row = true, .background = {10, 20, 30, 255}}};
    const cui_box content[] = {
        {.width = 240, .height = 80, .padding = 2, .background = {30, 20, 10, 255}}};
    opened[0] = true;
    opened[1] = opened[2] = false;
    cui_accordion_options options = {
        .collapsible = true, .trigger_boxes = trigger, .panel_boxes = content, .style_count = 1};
    assert(!scene(ui, options, &theme));
    assert(cui_bounds(ui, 10).w == 200 && cui_bounds(ui, 10).h == 56);
    assert(cui_bounds(ui, 30).h == theme.control_height);
    cui_rect panel_bounds = cui_bounds(ui, 10 ^ UINT64_C(0x1d8f322539deca61));
    assert(panel_bounds.w == 240 && panel_bounds.h == 80);
    click(ui, 10);
    assert(scene(ui, options, &theme) && !opened[0] && panels == 0);
    bool open = true;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_collapsible_ex(ui, 200, &theme, cui_string("More"), &open,
                       (cui_collapsible_options){.trigger_box = trigger, .panel_box = content},
                       panel, NULL);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_bounds(ui, 200).w == 200 && cui_bounds(ui, 200).h == 56);
    panel_bounds = cui_bounds(ui, 200 ^ UINT64_C(0x1d8f322539deca61));
    assert(panel_bounds.w == 240 && panel_bounds.h == 80);
    bool header_color = false, panel_color = false;
    for (size_t i = 0; i < draw.count; i++) {
        cui_draw_command command = draw.commands[i];
        header_color |= command.type == CUI_DRAW_RECT && command.color.r == 10 &&
                        command.color.g == 20 && command.color.b == 30;
        panel_color |= command.type == CUI_DRAW_RECT && command.color.r == 30 &&
                       command.color.g == 20 && command.color.b == 10;
    }
    assert(header_color && panel_color);
    click(ui, 200);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible_ex(
        ui, 200, &theme, cui_string("More"), &open,
        (cui_collapsible_options){.trigger_box = trigger, .panel_box = content}, panel, NULL));
    cui_end_frame(ui);
    assert(!open);
    cui_destroy(ui);
}
static float animated_height = 60;
static cui_spring_settings animated_spring = {.18f, 1, .001f, true};
static bool animated_activated;
static void animated_panel(cui_context *ui, size_t index, void *user) {
    (void)index;
    (void)user;
    cui_begin_box(ui, 801,
                  (cui_box){.width = 120, .height = animated_height, .background = {3, 4, 5, 255}});
    cui_interaction interaction = cui_base_button(ui, 801, cui_string("Body"), false);
    animated_activated |= interaction.activated;
    cui_end_box(ui);
}
static cui_draw_list animated_scene(cui_context *ui, bool *open, cui_disclosure_motion *motion,
                                    bool reduced, float dt) {
    assert(cui_begin_frame(ui, 640, 480, dt));
    cui_theme theme = cui_theme_light();
    theme.spring_control = animated_spring;
    cui_collapsible_ex(
        ui, 800, &theme, cui_string("Animated"), open,
        (cui_collapsible_options){.motion = motion, .dt = dt, .reduced_motion = reduced},
        animated_panel, NULL);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static bool has_semantic(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == id)
            return true;
    return false;
}
static void test_measured_reveal(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    bool open = false;
    cui_disclosure_motion motion = {0};
    animated_height = 60;
    animated_activated = false;
    cui_draw_list draw = animated_scene(ui, &open, &motion, false, .016f);
    assert(motion.progress == 0 && motion.measured.h >= 60);
    assert(!has_semantic(ui, 801));
    for (size_t i = 0; i < draw.count; i++)
        assert(!(draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 3 &&
                 draw.commands[i].color.g == 4));
    click(ui, 800);
    animated_scene(ui, &open, &motion, false, .016f);
    assert(open && motion.progress > 0 && motion.progress < 1);
    cui_id viewport = 800 ^ UINT64_C(0x48aaf2a21d3ed09f);
    assert(cui_bounds(ui, viewport).h > 0 && cui_bounds(ui, viewport).h < motion.measured.h);
    for (int i = 0; i < 60; i++)
        animated_scene(ui, &open, &motion, false, .016f);
    assert(motion.progress == 1 && has_semantic(ui, 801));
    cui_focus(ui, 801);
    cui_rect body = cui_bounds(ui, 801);
    open = false;
    draw = animated_scene(ui, &open, &motion, false, .016f);
    assert(motion.progress > 0 && motion.progress < 1);
    assert(!has_semantic(ui, 801) && cui_focused(ui) == 0);
    bool closing_paint = false;
    for (size_t i = 0; i < draw.count; i++)
        closing_paint |= draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 3 &&
                         draw.commands[i].color.g == 4;
    assert(closing_paint);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = body.x + 2, .y = body.y + 2}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = body.x + 2, .y = body.y + 2}));
    animated_scene(ui, &open, &motion, false, .016f);
    assert(!animated_activated);
    float before = motion.progress;
    open = true;
    for (int i = 0; i < 5; i++)
        animated_scene(ui, &open, &motion, false, .016f);
    assert(motion.progress > before && motion.progress < 1);
    animated_height = 100;
    animated_scene(ui, &open, &motion, true, .016f);
    assert(motion.progress == 1 && motion.measured.h >= 100);
    animated_scene(ui, &open, &motion, true, .016f);
    assert(cui_bounds(ui, viewport).h == motion.measured.h);
    open = false;
    animated_scene(ui, &open, &motion, true, .016f);
    assert(motion.progress == 0 && cui_bounds(ui, viewport).h == 0);
    cui_destroy(ui);
    cui_disclosure_motion pure = {0};
    cui_disclosure_motion_step(&pure, false, .016f, 0, false);
    cui_disclosure_motion_step(&pure, true, .05f, .2f, false);
    before = pure.progress;
    cui_disclosure_motion_step(&pure, true, -1, .2f, false);
    assert(pure.progress == before);
    cui_disclosure_motion_step(&pure, true, 100, .2f, false);
    assert(pure.progress == 1 && pure.velocity == 0);
}
static void test_accordion_reveal_storage(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_disclosure_motion motion[1] = {{0}};
    cui_accordion_options options = {
        .collapsible = true, .motion = motion, .motion_count = 1, .dt = .016f};
    for (size_t i = 0; i < 3; i++)
        opened[i] = false;
    scene(ui, options, &theme);
    assert(panels == 1 && motion[0].measured.h > 0 && !has_semantic(ui, 100));
    click(ui, 10);
    assert(scene(ui, options, &theme));
    assert(opened[0] && motion[0].progress > 0 && motion[0].progress < 1);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &theme));
    assert(!opened[0] && opened[2] && panels == 5);
    assert(!has_semantic(ui, 100) && has_semantic(ui, 102));
    options.reduced_motion = true;
    scene(ui, options, &theme);
    assert(motion[0].progress == 0);
    cui_destroy(ui);
}
static void test_configured_disclosure_springs(void) {
    cui_disclosure_motion fast = {0}, slow = {0}, under = {0}, over = {0};
    cui_spring_settings settings = {
        .response = .2f, .damping = 1, .epsilon = .001f, .travel = true};
    cui_disclosure_motion_step_ex(&fast, false, .016f, settings, false);
    slow = under = over = fast;
    cui_disclosure_motion_step_ex(&fast, true, .05f, settings, false);
    settings.response = .8f;
    cui_disclosure_motion_step_ex(&slow, true, .05f, settings, false);
    assert(fast.progress > slow.progress && slow.progress > 0);
    settings.response = .2f;
    settings.damping = 2;
    cui_disclosure_motion_step_ex(&over, true, .05f, settings, false);
    assert(over.progress < fast.progress && over.progress > 0);
    settings.damping = .2f;
    bool overshot = false;
    for (int i = 0; i < 12; i++) {
        cui_disclosure_motion_step_ex(&under, true, .016f, settings, false);
        overshot |= under.progress > 1;
    }
    assert(overshot);
    settings.travel = false;
    cui_disclosure_motion_step_ex(&under, false, 0, settings, false);
    assert(under.progress == 0 && under.velocity == 0);
    settings.travel = true;
    settings.response = 0;
    cui_disclosure_motion_step_ex(&under, true, 0, settings, false);
    assert(under.progress == 1 && under.velocity == 0);
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_disclosure_motion motion = {0};
    bool open = false;
    animated_spring =
        (cui_spring_settings){.response = .18f, .damping = 1, .epsilon = .001f, .travel = false};
    animated_scene(ui, &open, &motion, false, .016f);
    click(ui, 800);
    animated_scene(ui, &open, &motion, false, .016f);
    assert(open && motion.progress == 1);
    animated_spring.travel = true;
    animated_spring.response = 0;
    click(ui, 800);
    animated_scene(ui, &open, &motion, false, .016f);
    assert(!open && motion.progress == 0);
    animated_spring = (cui_spring_settings){.18f, 1, .001f, true};
    cui_destroy(ui);
}
static void test_disclosure_chevron(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
    cui_theme t = cui_theme_light();
    cui_disclosure_item item = {995, {"Details", 7}, false};
    bool open = false;
    unsigned char first[16 * 16 * 4];
    size_t bytes = 0;
    for (unsigned phase = 0; phase < 3; phase++) {
        if (phase == 1)
            click(ui, item.id);
        assert(cui_begin_frame(ui, 300, 100, .016f));
        cui_accordion(ui, 994, &t, &item, 1, &open, (cui_accordion_options){.disabled = phase == 2},
                      NULL, NULL);
        cui_draw_list draw = cui_end_frame(ui);
        unsigned images = 0;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_IMAGE) {
                cui_bitmap bitmap = draw.commands[i].bitmap.bitmap;
                images++;
                cui_rect trigger = cui_bounds(ui, item.id);
                assert(fabsf(draw.commands[i].bounds.y + draw.commands[i].bounds.h / 2 - trigger.y -
                             trigger.h / 2) < .1f);
                assert(draw.commands[i].bitmap.tint.r == t.muted.r);
                size_t length = bitmap.height * bitmap.stride;
                assert(length <= sizeof(first));
                if (!phase) {
                    bytes = length;
                    memcpy(first, bitmap.pixels, bytes);
                } else
                    assert(open && (length != bytes || memcmp(first, bitmap.pixels, bytes)));
            }
        assert(images == (phase == 2 ? 0u : 1u));
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

typedef struct {
    bool nested_open;
    size_t changes;
    cui_disclosure_change last;
} disclosure_presentation_state;

static void record_presentation_change(void *user, const cui_disclosure_change *change) {
    disclosure_presentation_state *state = user;
    state->last = *change;
    ++state->changes;
}

static void present_nested(cui_context *ui, size_t index, void *user) {
    if (index != 0)
        return;
    disclosure_presentation_state *state = user;
    const cui_theme theme = cui_theme_light();
    cui_collapsible_present(
        ui, 2002, &theme, cui_string("Nested details"), &state->nested_open,
        (cui_collapsible_options){0},
        (cui_disclosure_presentation){.width = -1, .design_size = CUI_DESIGN_XS}, panel, NULL,
        (cui_disclosure_handler){record_presentation_change, state});
}

static void test_disclosure_presentation(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 700});
    const cui_theme theme = cui_theme_light();
    const cui_disclosure_item section = {
        2001, cui_string("A long heading keeps its accessible name at narrow widths"), false};
    bool open = false;
    disclosure_presentation_state state = {0};
    assert(ui);
    for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; ++size) {
        assert(cui_begin_frame(ui, 400, 700, .016f));
        const float width = size == CUI_DESIGN_9XL ? 360 : 280;
        cui_begin_box(ui, 2000, (cui_box){.width = width});
        assert(
            !cui_accordion_present(
                 ui, 2003, &theme, &section, 1, &open, (cui_accordion_options){.bordered = true},
                 (cui_disclosure_presentation){.width = -1, .design_size = (cui_design_size)size},
                 NULL, NULL, (cui_disclosure_handler){0})
                 .changed);
        cui_end_box(ui);
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_design_metrics metrics;
        assert(cui_design_measure((cui_design_size)size, theme.font_size / 16, &metrics));
        cui_rect row = cui_bounds(ui, section.id);
        assert(row.w == width && row.h == metrics.control_height);
        size_t images = 0, titles = 0, title_clips = 0, semantic_count;
        const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
        assert(semantic_count == 2 && semantics[0].id == section.id &&
               semantics[0].name.length == section.label.length);
        for (size_t i = 0; i < draw.count; ++i) {
            const cui_draw_command *cmd = &draw.commands[i];
            if (cmd->type == CUI_DRAW_IMAGE) {
                ++images;
                assert(fabsf(cmd->bounds.y + cmd->bounds.h / 2 - row.y - row.h / 2) < .6f);
                assert(fabsf(cmd->bounds.x + cmd->bounds.w - row.x - row.w + metrics.padding_x) <
                       .6f);
            } else if (cmd->type == CUI_DRAW_TEXT) {
                ++titles;
                assert(cmd->font_size == metrics.font_size);
            } else if (cmd->type == CUI_DRAW_CLIP_PUSH) {
                ++title_clips;
                assert(cmd->bounds.h == metrics.line_height);
                assert(fabsf(cmd->bounds.y + cmd->bounds.h / 2 - row.y - row.h / 2) < .6f);
            }
        }
        assert(images == 1 && titles == 1 && title_clips == 1);
    }

    cui_disclosure_presentation p = {.width = 280,
                                     .design_size = CUI_DESIGN_BASE,
                                     .collapsed_icon = {"plus", 4},
                                     .expanded_icon = {"minus", 5}};
    for (int phase = 0; phase < 4; ++phase) {
        if (phase > 0)
            click(ui, phase == 2 ? 2002 : section.id);
        assert(cui_begin_frame(ui, 400, 700, .016f));
        cui_disclosure_event_result result = cui_accordion_present(
            ui, 2003, &theme, &section, 1, &open, (cui_accordion_options){.collapsible = true}, p,
            present_nested, &state, (cui_disclosure_handler){record_presentation_change, &state});
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(result.changed == (phase == 1 || phase == 3));
        assert(open == (phase == 1 || phase == 2));
        assert(has_semantic(ui, 2002) == open);
        assert(state.changes == (size_t)phase);
        if (phase == 2)
            assert(state.nested_open && state.last.item == 2002 && state.last.expanded);
    }
    assert(cui_begin_frame(ui, 400, 700, .016f));
    p.width = NAN;
    assert(!cui_accordion_present(ui, 2003, &theme, &section, 1, &open, (cui_accordion_options){0},
                                  p, present_nested, &state,
                                  (cui_disclosure_handler){record_presentation_change, &state})
                .changed);
    assert(cui_end_frame(ui).count == 0 && !open && state.changes == 3);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static size_t count_paint(cui_draw_list list, cui_draw_type type, cui_rect bounds,
                          cui_color color) {
    size_t found = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *cmd = &list.commands[i];
        found += cmd->type == type && cmd->bounds.x == bounds.x && cmd->bounds.y == bounds.y &&
                 cmd->bounds.w == bounds.w && cmd->bounds.h == bounds.h &&
                 cmd->color.r == color.r && cmd->color.g == color.g && cmd->color.b == color.b &&
                 cmd->color.a == color.a;
    }
    return found;
}

static cui_draw_list joined_scene(cui_context *ui, const cui_theme *theme,
                                  cui_disclosure_presentation presentation, bool bordered,
                                  bool expanded[3]) {
    assert(cui_begin_frame(ui, 400, 480, .016f));
    (void)cui_accordion_present(ui, 2003, theme, items, 3, expanded,
                                (cui_accordion_options){.bordered = bordered, .collapsible = true},
                                presentation, panel, NULL, (cui_disclosure_handler){0});
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}

static void test_joined_disclosure_styles(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 480});
    const cui_theme theme = cui_theme_light();
    const cui_disclosure_trigger_style styles[] = {{.custom_hover = true,
                                                    .hover = {1, 50, 200, 255},
                                                    .custom_pressed = true,
                                                    .pressed = {2, 50, 200, 255},
                                                    .custom_expanded = true,
                                                    .expanded = {3, 50, 200, 255},
                                                    .custom_focus = true,
                                                    .focus = {4, 50, 200, 255},
                                                    .custom_cursor = true,
                                                    .cursor = CUI_CURSOR_CROSSHAIR},
                                                   {.custom_hover = true,
                                                    .hover = {5, 50, 200, 255},
                                                    .custom_disabled = true,
                                                    .disabled = {6, 50, 200, 255},
                                                    .custom_cursor = true,
                                                    .cursor = CUI_CURSOR_CROSSHAIR}};
    cui_disclosure_presentation p = {
        .width = 240, .joined = true, .trigger_styles = styles, .trigger_style_count = 2};
    bool expanded[] = {true, false, false};
    assert(ui);
    cui_draw_list list = joined_scene(ui, &theme, p, true, expanded);
    cui_rect outer = cui_bounds(ui, 2003), first = cui_bounds(ui, 10), second = cui_bounds(ui, 20);
    cui_rect third = cui_bounds(ui, 30);
    cui_rect content = cui_bounds(ui, 10 ^ UINT64_C(0x1d8f322539deca61));
    cui_rect divider = {first.x, content.y + content.h, first.w, 1};
    assert(outer.w == 240 && first.w == second.w && second.w == third.w && first.w == content.w);
    assert(content.y == first.y + first.h && second.y == divider.y + 1);
    assert(third.y == second.y + second.h + 1);
    assert(count_paint(list, CUI_DRAW_BORDER, outer, theme.border) == 1);
    assert(count_paint(list, CUI_DRAW_BORDER, first, theme.border) == 0);
    assert(count_paint(list, CUI_DRAW_RECT, divider, theme.border) == 1);
    assert(count_paint(list, CUI_DRAW_RECT, (cui_rect){third.x, third.y + third.h, third.w, 1},
                       theme.border) == 0);
    assert(count_paint(list, CUI_DRAW_RECT, first, styles[0].expanded) == 1);
    assert(count_paint(list, CUI_DRAW_RECT, second, styles[1].disabled) == 1);

    // Use different frames to observe live hover, capture, and release.
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = first.x + 2, .y = first.y + 2}));
    list = joined_scene(ui, &theme, p, true, expanded);
    assert(count_paint(list, CUI_DRAW_RECT, first, styles[0].hover) == 1);
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_CROSSHAIR);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = first.x + 2, .y = first.y + 2}));
    list = joined_scene(ui, &theme, p, true, expanded);
    assert(count_paint(list, CUI_DRAW_RECT, first, styles[0].pressed) == 1);
    assert(count_paint(list, CUI_DRAW_BORDER, first, styles[0].focus) == 1);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = second.x + 2, .y = second.y + 2}));
    list = joined_scene(ui, &theme, p, true, expanded);
    assert(count_paint(list, CUI_DRAW_RECT, first, styles[0].expanded) == 1);
    assert(count_paint(list, CUI_DRAW_RECT, second, styles[1].disabled) == 1);
    assert(count_paint(list, CUI_DRAW_RECT, second, styles[1].hover) == 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = second.x + 2, .y = second.y + 2}));
    list = joined_scene(ui, &theme, p, true, expanded);
    assert(expanded[0] && cui_pointer_cursor(ui) != CUI_CURSOR_CROSSHAIR);
    key(ui, CUI_KEY_SPACE);
    list = joined_scene(ui, &theme, p, true, expanded);
    assert(count_paint(list, CUI_DRAW_RECT, first, styles[0].pressed) == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    joined_scene(ui, &theme, p, true, expanded);
    assert(!expanded[0]);

    // Intrinsic width fits the longest trigger, including its icon.
    p.width = 0;
    list = joined_scene(ui, &theme, p, false, expanded);
    outer = cui_bounds(ui, 2003);
    first = cui_bounds(ui, 10);
    third = cui_bounds(ui, 30);
    assert(first.w > 32 && first.w == third.w && third.w == outer.w);
    assert(count_paint(list, CUI_DRAW_BORDER, outer, theme.border) == 0);
    assert(count_paint(list, CUI_DRAW_RECT, (cui_rect){first.x, first.y + first.h, first.w, 1},
                       theme.border) == 1);
    cui_box container = {.width = 260, .padding = 6, .radius = 8, .background = {2, 19, 72, 255}};
    p.container_box = &container;
    list = joined_scene(ui, &theme, p, true, expanded);
    outer = cui_bounds(ui, 2003);
    first = cui_bounds(ui, 10);
    assert(outer.w == 260 && first.w == 248 && first.x == outer.x + 6);
    assert(count_paint(list, CUI_DRAW_RECT, outer, container.background) == 1);

    assert(cui_begin_frame(ui, 400, 480, .016f));
    bool open = true;
    (void)cui_collapsible_present(ui, 2500, &theme, cui_string("Collapsible"), &open,
                                  (cui_collapsible_options){0}, p, panel, NULL,
                                  (cui_disclosure_handler){0});
    list = cui_end_frame(ui);
    cui_rect solo = cui_bounds(ui, 2500);
    assert(cui_last_error(ui) == CUI_OK && solo.w == 248);
    assert(count_paint(list, CUI_DRAW_RECT, solo, styles[0].expanded) == 1);
    assert(count_paint(list, CUI_DRAW_BORDER, solo, theme.border) == 1);

    // Hover paint uses the same corner normalization as the ordinary box fill.
    const cui_box trigger = {.width = -1,
                             .height = 36,
                             .row = true,
                             .radius = NAN,
                             .custom_corners = true,
                             .corner_radii = {-1, 4, INFINITY, 3}};
    assert(cui_begin_frame(ui, 400, 480, .016f));
    (void)cui_collapsible_present(ui, 2500, &theme, cui_string("Collapsible"), &open,
                                  (cui_collapsible_options){.trigger_box = &trigger}, p, panel,
                                  NULL, (cui_disclosure_handler){0});
    list = cui_end_frame(ui);
    solo = cui_bounds(ui, 2500);
    assert(cui_last_error(ui) == CUI_OK);
    assert(count_paint(list, CUI_DRAW_RECT, solo, styles[0].expanded) == 1);
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *cmd = &list.commands[i];
        if (cmd->type == CUI_DRAW_RECT && cmd->color.r == styles[0].expanded.r &&
            cmd->color.b == styles[0].expanded.b) {
            assert(cmd->custom_corners);
            assert(cmd->corner_radii.top_left == 0 && cmd->corner_radii.top_right == 4 &&
                   cmd->corner_radii.bottom_left == 0 && cmd->corner_radii.bottom_right == 3);
        }
    }
    cui_destroy(ui);
}

int main(void) {
    test_disclosure_chevron();
    test_disclosure_presentation();
    test_joined_disclosure_styles();
    test_measured_reveal();
    test_configured_disclosure_springs();
    test_accordion_reveal_storage();
    test_custom_disclosure_headers();
    test_disclosure_box_overrides();
    for (size_t i = 0; i < 3; i++)
        opened[i] = false;
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_accordion_options options = {.collapsible = true, .bordered = true};
    for (size_t i = 0; i < 4; i++)
        assert(!scene(ui, options, &themes[i]));
    assert(!panels);
    click(ui, 10);
    assert(scene(ui, options, &themes[0]));
    assert(opened[0] && panels == 1);
    size_t count = 0;
    const cui_semantic *s = cui_semantics(ui, &count);
    assert(count == 6 && s[0].checked && s[0].role == CUI_ROLE_BUTTON && s[1].disabled &&
           s[0].details.expanded);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_DOWN);
    assert(!scene(ui, options, &themes[0]));
    assert(cui_focused(ui) == 30);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &themes[0]));
    assert(!opened[0] && opened[2] && panels == 4);
    options.collapsible = false;
    key(ui, CUI_KEY_ENTER);
    assert(!scene(ui, options, &themes[0]));
    assert(opened[2]);
    key(ui, CUI_KEY_HOME);
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 10);
    key(ui, CUI_KEY_END);
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 30);
    options.multiple = true;
    options.collapsible = true;
    click(ui, 10);
    assert(scene(ui, options, &themes[0]));
    assert(opened[0] && opened[2] && panels == 5);
    click(ui, 20);
    assert(!scene(ui, options, &themes[0]));
    assert(!opened[1]);
    options.disabled = true;
    click(ui, 10);
    assert(!scene(ui, options, &themes[0]));
    assert(opened[0]);
    options.disabled = false;
    cui_focus(ui, 100);
    opened[0] = false;
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 0); /* hidden content cannot remain focused */
    bool open = false;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_collapsible(ui, 200, &themes[0], cui_string("More"), &open, false, panel, NULL);
    cui_end_frame(ui);
    cui_focus(ui, 200);
    key(ui, CUI_KEY_SPACE);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible(ui, 200, &themes[0], cui_string("More"), &open, false, panel, NULL));
    cui_end_frame(ui);
    assert(open);
    cui_focus(ui, 200);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ENTER);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible(ui, 200, &themes[0], cui_string("More"), &open, false, panel, NULL));
    cui_end_frame(ui);
    assert(open); /* two activations survive aggregation */
    opened[0] = true;
    opened[2] = false;
    scene(ui, options, &themes[0]);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &themes[0]));
    assert(opened[2] && cui_focused(ui) == 30);
    cui_destroy(ui);
    return 0;
}
