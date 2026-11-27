/* SPDX-License-Identifier: MIT */
#undef NDEBUG
#include "cui/cui_button_styles.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>

static const cui_color clear = {0, 0, 0, 255};
static const cui_color fill = {20, 40, 60, 255};
static const cui_color ink = {255, 240, 220, 255};
static const cui_color ring = {160, 210, 250, 255};

static void click(void *user) {
    ++*(int *)user;
}

static cui_draw_list scene(cui_context *ui, const cui_theme *theme, cui_button_options options,
                           cui_button_focus_mode mode, bool clip, bool expected_activation) {
    assert(cui_begin_frame(ui, 200, 120, .016f));
    cui_begin_box(ui, 1, (cui_box){.padding = 20});
    cui_begin_box(ui, 2, (cui_box){.width = 130, .height = 80, .clip = clip});
    assert(cui_button_focus_ex(ui, 3, theme, cui_string("Save"), options, mode) ==
           expected_activation);
    cui_begin_box(ui, 4, (cui_box){.width = 25, .height = 20});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_rect button = cui_bounds(ui, 3), sibling = cui_bounds(ui, 4);
    assert(button.x == 20 && button.y == 20 && button.w == 80 && button.h == 32 && sibling.y == 52);
    return draw;
}

static const cui_draw_command *border(cui_draw_list draw, unsigned red) {
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *command = &draw.commands[i];
        if (command->type == CUI_DRAW_BORDER && command->color.r == red)
            return command;
    }
    return NULL;
}

static void paint(cui_software *software, cui_draw_list draw) {
    cui_draw_command commands[24];
    cui_draw_list visible = {.commands = commands, .width = draw.width, .height = draw.height};
    assert(draw.count <= sizeof commands / sizeof commands[0]);
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[visible.count++] = draw.commands[i];
    assert(cui_software_render(software, &visible, clear));
}

static unsigned red_at(cui_software *software, unsigned x, unsigned y) {
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(software, &width, &height);
    assert(x < width && y < height);
    return pixels[(y * width + x) * 4];
}

static void empty_inset_uses_no_command(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 16, .max_commands = 1});
    assert(ui && cui_begin_frame(ui, 40, 40, .016f));
    cui_begin_box(ui, 5, (cui_box){.width = 12, .height = 12, .background = fill});
    assert(cui_set_outline_offset(ui, 1, -7, ring, (cui_corner_radii){0}));
    assert(!cui_set_outline_offset(ui, 1, NAN, ring, (cui_corner_radii){0}));
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draw.count == 1 &&
           draw.commands[0].type == CUI_DRAW_RECT);
    cui_destroy(ui);
}

int main(void) {
    empty_inset_uses_no_command();
    cui_context *ui = cui_create((cui_config){0});
    cui_software *software = cui_software_create(200, 120, NULL);
    assert(ui && software);
    cui_theme theme = cui_theme_dark();
    theme.font_size = 16;
    theme.focus = ring;
    int clicks = 0;
    cui_button_options options = {.width = 80,
                                  .height = 32,
                                  .custom_colors = true,
                                  .colors = {.normal = fill, .foreground = ink, .border = fill},
                                  .custom_corners = true,
                                  .corner_radii = {8, 6, 2, 10},
                                  .on_click = click,
                                  .action_user = &clicks};
    assert(!border(scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, false, false), ring.r));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    cui_draw_list draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, false, false);
    assert(cui_focused(ui) == 3);
    const cui_draw_command *line = border(draw, ink.r);
    assert(line && line->stroke == 1 && line->color.a == 153 && line->bounds.x == 22 &&
           line->bounds.y == 22 && line->bounds.w == 76 && line->corner_radii.top_left == 6 &&
           line->corner_radii.bottom_left == 0);
    paint(software, draw);
    assert(red_at(software, 60, 22) > 100 && red_at(software, 60, 32) == fill.r);

    options.variant = CUI_BUTTON_DEFAULT;
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, false, false);
    line = border(draw, ring.r);
    assert(line && line->stroke == 1 && line->bounds.x == 20);
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_LEGACY, false, false);
    line = border(draw, ring.r);
    assert(line && line->stroke == 2 && line->bounds.x == 20);

    options.variant = CUI_BUTTON_GHOST;
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, false, false);
    line = border(draw, ring.r);
    assert(line && line->stroke == 1 && line->bounds.x == 20 && line->bounds.w == 80);
    options.variant = CUI_BUTTON_LINK;
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, false, false);
    line = border(draw, ring.r);
    assert(line && line->bounds.x == 17 && line->bounds.w == 86);
    paint(software, draw);
    assert(red_at(software, 17, 36) == ring.r && red_at(software, 19, 36) == clear.r);
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_VARIANT, true, false);
    paint(software, draw);
    assert(red_at(software, 17, 36) == clear.r && red_at(software, 82, 54) == ring.r);

    options.variant = CUI_BUTTON_PRIMARY;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_OUTER_RING, false, true);
    line = border(draw, ring.r);
    assert(clicks == 1 && line && line->stroke == 3 && line->color.a == 127 &&
           line->bounds.x == 17 && line->bounds.w == 86);
    paint(software, draw);
    assert(red_at(software, 18, 36) > 60 && red_at(software, 60, 33) == fill.r);
    options.colors.normal.a = 128;
    paint(software, scene(ui, &theme, options, CUI_BUTTON_FOCUS_OUTER_RING, false, false));
    unsigned tinted = red_at(software, 60, 33);
    assert(tinted > clear.r && tinted < fill.r && red_at(software, 18, 36) > 60);
    options.hide_focus_ring = true;
    draw = scene(ui, &theme, options, CUI_BUTTON_FOCUS_OUTER_RING, false, false);
    assert(!border(draw, ring.r));
    paint(software, draw);
    assert(red_at(software, 60, 33) == tinted);
    options.hide_focus_ring = false;
    options.disabled = true;
    assert(!border(scene(ui, &theme, options, CUI_BUTTON_FOCUS_OUTER_RING, false, false), ring.r));
    assert(clicks == 1);
    cui_software_destroy(software);
    cui_destroy(ui);
}
