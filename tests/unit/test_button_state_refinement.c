/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static cui_context *ui;
static cui_software *renderer;
static cui_theme theme;
static cui_button_options options;
static unsigned clicks;
static void click(void *user) {
    assert(user == &clicks);
    clicks++;
}
static cui_color alpha(cui_color c, float a) {
    float v = c.a * a;
    c.a = (unsigned char)(v > 255 ? 255 : v);
    return c;
}
static bool equal(cui_color a, cui_color b) {
    return !memcmp(&a, &b, sizeof a);
}
static cui_draw_list frame(void) {
    assert(cui_begin_frame(ui, 200, 80, .016f));
    (void)cui_button_ex(ui, 1, &theme, cui_string("X"), options);
    cui_draw_list draw = cui_end_frame(ui);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(count == 1 && nodes[0].id == 1);
    assert(nodes[0].disabled == (options.disabled || options.loading));
    assert(nodes[0].details.busy == options.loading);
    assert(!nodes[0].checked); /* selected/open remain presentation only */
    return draw;
}
static void event(cui_event_type type, bool inside) {
    cui_rect b = cui_bounds(ui, 1);
    assert(cui_queue_event(
        ui, (cui_event){.type = type, .x = inside ? b.x + 8 : 190, .y = inside ? b.y + 8 : 70}));
}
static void foreground(cui_draw_list draw, cui_color expected) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            assert(equal(draw.commands[i].color, expected));
            return;
        }
    assert(false);
}
/* Blank interior pixel verifies actual alpha composition, independently of text. */
static void background(cui_draw_list draw, cui_color expected) {
    cui_draw_command commands[128];
    cui_draw_list surfaces = {.commands = commands, .width = draw.width, .height = draw.height};
    assert(draw.count <= 128);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[surfaces.count++] = draw.commands[i];
    assert(cui_software_render(renderer, &surfaces, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    cui_rect b = cui_bounds(ui, 1);
    unsigned x = (unsigned)b.x + 30, y = (unsigned)(b.y + b.h * .5f);
    assert(x < w && y < h);
    const unsigned char *p = pixels + (y * w + x) * 4;
    unsigned channels[] = {expected.r, expected.g, expected.b};
    for (unsigned i = 0; i < 3; i++) {
        int wanted = (int)(channels[i] * expected.a / 255);
        assert((int)p[i] >= wanted - 1 && (int)p[i] <= wanted + 1);
    }
}
static void border(cui_draw_list draw, cui_color expected) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_BORDER) {
            assert(equal(draw.commands[i].color, expected));
            return;
        }
    assert(expected.a == 0);
}
int main(void) {
    ui = cui_create((cui_config){.width = 200, .height = 80});
    renderer = cui_software_create(200, 80, NULL);
    assert(ui && renderer);
    for (unsigned dark = 0; dark < 2; dark++) {
        theme = dark ? cui_theme_dark() : cui_theme_light();
        /* Distinct translucent tokens make wrong state/alpha observable. */
        theme.accent = (cui_color){100, 150, 200, 180};
        theme.accent_foreground = (cui_color){1, 2, 3, 255};
        theme.secondary_foreground = (cui_color){210, 220, 230, 255};
        options = (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                       .width = 120,
                                       .hide_focus_ring = true,
                                       .on_click = click,
                                       .action_user = &clicks};
        frame();
        event(CUI_EVENT_POINTER_MOVE, true);
        cui_draw_list draw = frame();
        background(draw, alpha(theme.accent, dark ? .5f : 1));
        foreground(draw, theme.accent_foreground);
        event(CUI_EVENT_POINTER_DOWN, true);
        draw = frame();
        background(draw, theme.buttons[CUI_THEME_BUTTON_DEFAULT].active);
        foreground(draw, theme.secondary_foreground);
        event(CUI_EVENT_CANCEL, true);
        frame();
        options.selected = true;
        draw = frame();
        background(draw, theme.buttons[CUI_THEME_BUTTON_SECONDARY].active);
        foreground(draw, theme.secondary_foreground);
        options.selected = false;
        options.open = true;
        draw = frame();
        foreground(draw, theme.secondary_foreground);
        options.outline = true;
        draw = frame();
        background(draw, theme.buttons[CUI_THEME_BUTTON_DEFAULT].active);
        foreground(draw, theme.secondary_foreground);
        options.open = false;
        event(CUI_EVENT_POINTER_MOVE, false);
        frame();
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_CUSTOM; variant++) {
            options = (cui_button_options){.variant = (cui_button_variant)variant,
                                           .width = 120,
                                           .hide_focus_ring = true,
                                           .custom_colors = true,
                                           .colors = {.normal = {40, 80, 120, 180},
                                                      .hover = {60, 90, 130, 180},
                                                      .active = {80, 100, 140, 180},
                                                      .border = {20, 30, 40, 120},
                                                      .outline_normal = {50, 70, 90, 160},
                                                      .outline_border = {70, 80, 90, 150},
                                                      .foreground = {200, 210, 220, 255},
                                                      .outline_foreground = {180, 190, 200, 255}}};
            for (unsigned outlined = 0; outlined < 2; outlined++) {
                options.outline = outlined != 0;
                options.disabled = true;
                options.loading = true; /* disabled wins over loading opacity */
                options.selected = true;
                draw = frame();
                bool line = variant == CUI_BUTTON_GHOST || variant == CUI_BUTTON_LINK ||
                            variant == CUI_BUTTON_TEXT;
                bool outline = outlined || variant == CUI_BUTTON_OUTLINE;
                float opacity = outline ? .5f : variant == CUI_BUTTON_SECONDARY ? 1.5f : .15f;
                cui_color bg =
                    alpha(outline ? options.colors.outline_normal : options.colors.normal, opacity);
                if (line)
                    bg.a = 0;
                background(draw, bg);
                if (!line || outline)
                    border(draw,
                           alpha(outline ? options.colors.outline_border : options.colors.border,
                                 opacity));
                foreground(draw, alpha(theme.muted, .5f));
                options.disabled = options.loading = options.selected = false;
                draw = frame(); /* recovery restores normal palette */
                background(draw, outline ? options.colors.outline_normal : options.colors.normal);
            }
        }
        /* Real default palettes: variant border rules, without caller overrides. */
        const cui_theme_button_kind kinds[] = {
            CUI_THEME_BUTTON_PRIMARY, CUI_THEME_BUTTON_SECONDARY, CUI_THEME_BUTTON_DEFAULT,
            CUI_THEME_BUTTON_PRIMARY, CUI_THEME_BUTTON_DANGER,    CUI_THEME_BUTTON_PRIMARY,
            CUI_THEME_BUTTON_WARNING, CUI_THEME_BUTTON_SUCCESS,   CUI_THEME_BUTTON_INFO,
            CUI_THEME_BUTTON_DEFAULT, CUI_THEME_BUTTON_PRIMARY,   CUI_THEME_BUTTON_PRIMARY};
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_CUSTOM; variant++) {
            for (unsigned outlined = 0; outlined < 2; outlined++) {
                options = (cui_button_options){.variant = (cui_button_variant)variant,
                                               .width = 120,
                                               .disabled = true,
                                               .outline = outlined != 0,
                                               .hide_focus_ring = true};
                bool outline = outlined || variant == CUI_BUTTON_OUTLINE;
                bool line = variant == CUI_BUTTON_GHOST || variant == CUI_BUTTON_LINK ||
                            variant == CUI_BUTTON_TEXT;
                cui_theme_button_colors colors = theme.buttons[kinds[variant]];
                float opacity = outline ? .5f : variant == CUI_BUTTON_SECONDARY ? 1.5f : .15f;
                cui_color edge = alpha(outline ? colors.outline_border : colors.border, opacity);
                if (line || variant == CUI_BUTTON_CUSTOM)
                    edge = (cui_color){0};
                if (variant == CUI_BUTTON_DEFAULT && !outline)
                    edge = alpha(theme.input, .5f);
                draw = frame();
                border(draw, edge);
                foreground(draw, alpha(theme.muted, .5f));
            }
        }
        /* Loading keeps selected colors, with whole-control fade; recover normal hover. */
        options = (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                       .width = 120,
                                       .selected = true,
                                       .loading = true,
                                       .hide_focus_ring = true};
        draw = frame();
        background(draw, alpha(theme.buttons[CUI_THEME_BUTTON_SECONDARY].active, .8f));
        options.selected = options.loading = false;
        event(CUI_EVENT_POINTER_MOVE, true);
        draw = frame();
        background(draw, alpha(theme.accent, dark ? .5f : 1));
        event(CUI_EVENT_POINTER_MOVE, false);
        frame();
        /* Outlined selected Link/Text keep normal ink, unlike filled selected ink. */
        for (unsigned text = 0; text < 2; text++) {
            options = (cui_button_options){.variant = text ? CUI_BUTTON_TEXT : CUI_BUTTON_LINK,
                                           .width = 120,
                                           .selected = true,
                                           .outline = true,
                                           .hide_focus_ring = true};
            foreground(frame(), text ? alpha(theme.text, .9f) : theme.link);
            options.outline = false;
            foreground(frame(), text ? alpha(theme.text, .7f) : theme.link_active);
        }
    }
    assert(clicks == 0); /* cancellation and inert states did not activate */
    cui_software_destroy(renderer);
    cui_destroy(ui);
    puts("button state refinement: ok");
}
