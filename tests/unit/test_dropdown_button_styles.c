/* SPDX-License-Identifier: MIT */
/* Private inclusion exposes the paint-only resolver for independent formula
 * checks; the static archive does not pull its duplicate choice object. */
#include "../../src/components/cui_choice.c"
#include "../fixtures/dropdown_button_styles/legacy_box.h"
#include "cui/cui_software.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static bool eq(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static cui_color alpha(cui_color c, float factor) {
    c.a = (uint8_t)(c.a * factor);
    return c;
}
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){.width = 360, .height = 180});
    CHECK(ui);
    cui_set_reduced_motion(ui, true);
    return ui;
}
static cui_menu_entry entries[] = {{.id = 12, .label = {"Export", 6}, .kind = CUI_MENU_ACTION}};
static cui_id command;
static cui_draw_list frame(cui_context *ui, cui_menu_tree_state *menu,
                           cui_dropdown_button_options o, const cui_dropdown_button_styles *styles,
                           bool old) {
    cui_theme t = cui_theme_light();
    CHECK(cui_begin_frame(ui, 360, 180, .016f));
    o.menu.popup.viewport = (cui_rect){0, 0, 360, 180};
    if (old)
        (void)legacy_render(ui, 1, 2, 3, &t, cui_string("Save"), cui_string("More"), entries, 1,
                            &command, menu, o, (cui_dropdown_event_handlers){0}, NULL);
    else
        (void)cui_dropdown_button_styled_ex(ui, 1, 2, 3, &t, cui_string("Save"), cui_string("More"),
                                            entries, 1, &command, menu, o, styles);
    cui_draw_list draw = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void pixels(cui_draw_list draw, unsigned char out[360 * 180 * 4]) {
    cui_draw_command commands[512];
    cui_draw_list copy = {.commands = commands, .width = 360, .height = 180};
    CHECK(draw.count < 512);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(360, 180, NULL);
    CHECK(sw);
    CHECK(cui_software_render(sw, &copy, (cui_color){255, 255, 255, 255}));
    memcpy(out, cui_software_pixels(sw, NULL, NULL), 360 * 180 * 4);
    cui_software_destroy(sw);
}
static void compare(cui_context *a, cui_context *b, cui_draw_list da, cui_draw_list db) {
    CHECK(da.count == db.count);
    for (size_t i = 0; i < da.count; i++) {
        const cui_draw_command *x = &da.commands[i], *y = &db.commands[i];
        CHECK(x->type == y->type && eq(x->color, y->color));
        CHECK(x->bounds.x == y->bounds.x && x->bounds.y == y->bounds.y &&
              x->bounds.w == y->bounds.w && x->bounds.h == y->bounds.h);
        CHECK(x->text.length == y->text.length);
        CHECK(!x->text.length || !memcmp(x->text.data, y->text.data, x->text.length));
    }
    unsigned char pa[360 * 180 * 4], pb[360 * 180 * 4];
    pixels(da, pa);
    pixels(db, pb);
    CHECK(!memcmp(pa, pb, sizeof pa));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
    CHECK(na == nb);
    for (size_t i = 0; i < na; i++) {
        CHECK(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
              sa[i].disabled == sb[i].disabled && sa[i].checked == sb[i].checked &&
              sa[i].focused == sb[i].focused);
        CHECK(sa[i].name.length == sb[i].name.length &&
              !memcmp(sa[i].name.data, sb[i].name.data, sa[i].name.length));
        CHECK(sa[i].details.expanded == sb[i].details.expanded &&
              sa[i].details.busy == sb[i].details.busy);
        CHECK(sa[i].bounds.x == sb[i].bounds.x && sa[i].bounds.y == sb[i].bounds.y &&
              sa[i].bounds.w == sb[i].bounds.w && sa[i].bounds.h == sb[i].bounds.h);
    }
}
static void legacy_zero(void) {
    cui_context *a = make(), *b = make();
    cui_menu_tree_state ma, mb;
    cui_menu_tree_init(&ma, 10);
    cui_menu_tree_init(&mb, 10);
    cui_dropdown_button_styles zero = {0};
    for (unsigned size = 0; size < 4; size++)
        for (int v = CUI_DROPDOWN_BUTTON_DEFAULT; v <= CUI_DROPDOWN_BUTTON_CUSTOM; v++)
            for (unsigned state = 0; state < 6; state++) {
                cui_dropdown_button_options o = {.variant = (cui_dropdown_button_variant)v,
                                                 .width = 130,
                                                 .size = (cui_dropdown_button_size)size,
                                                 .selected = state == 1,
                                                 .disabled = state == 2,
                                                 .action_loading = state == 3,
                                                 .outline = state == 4,
                                                 .hide_action = state == 5,
                                                 .custom = {{30, 60, 90, 200},
                                                            {40, 70, 100, 210},
                                                            {50, 80, 110, 220},
                                                            {240, 230, 220, 255}}};
                cui_draw_list da = frame(a, &ma, o, NULL, true),
                              db = frame(b, &mb, o, state & 1 ? &zero : NULL, false);
                compare(a, b, da, db);
                CHECK(cui_begin_frame(a, 360, 180, .016f));
                cui_color x, y;
                cui_dropdown_button_metrics m = cui_dropdown_button_size_metrics(o.size, 1);
                cui_theme t = cui_theme_light();
                cui_box old =
                    legacy_box(a, 2, &t, o, false, o.disabled || o.action_loading, false, &x, &m);
                cui_box current = cui_dropdown_button_box(
                    a, 2, &t, o, false, o.disabled || o.action_loading, false, &y, &m);
                CHECK(eq(old.background, current.background) && eq(old.border, current.border) &&
                      eq(x, y));
                CHECK(old.width == current.width && old.height == current.height &&
                      old.border_width == current.border_width);
                (void)cui_end_frame(a);
            }
    cui_destroy(a);
    cui_destroy(b);
}
static void source_formulas(void) {
    cui_dropdown_button_metrics m = cui_dropdown_button_size_metrics(CUI_DROPDOWN_BUTTON_MEDIUM, 1);
    for (unsigned dark = 0; dark < 2; dark++) {
        cui_theme t = dark ? cui_theme_dark() : cui_theme_light();
        t.accent = (cui_color){20, 40, 60, 200};
        cui_dropdown_button_options o = {.variant = CUI_DROPDOWN_BUTTON_GHOST};
        cui_color fg;
        cui_box b =
            cui_dropdown_style_box(&t, o, false, false, false, &fg, &m, true, false, false, true);
        CHECK(eq(b.background, alpha(t.accent, dark ? .5f : 1)) && eq(fg, t.accent_foreground));
        b = cui_dropdown_style_box(&t, o, false, false, true, &fg, &m, false, false, false, true);
        CHECK(eq(b.background, alpha(t.accent, dark ? .25f : .5f)));
        o.selected = true;
        b = cui_dropdown_style_box(&t, o, false, false, false, &fg, &m, true, true, false, true);
        CHECK(eq(b.background, t.buttons[CUI_THEME_BUTTON_SECONDARY].active) &&
              eq(fg, t.secondary_foreground));
        o.outline = true;
        b = cui_dropdown_style_box(&t, o, false, false, false, &fg, &m, true, true, false, true);
        CHECK(eq(b.background, t.buttons[CUI_THEME_BUTTON_DEFAULT].active));
        o.variant = CUI_DROPDOWN_BUTTON_LINK;
        b = cui_dropdown_style_box(&t, o, false, false, false, &fg, &m, true, true, false, true);
        CHECK(eq(fg, t.link));
        o.variant = CUI_DROPDOWN_BUTTON_TEXT;
        b = cui_dropdown_style_box(&t, o, false, false, false, &fg, &m, true, true, false, true);
        CHECK(eq(fg, alpha(t.text, .9f)));
        for (int v = CUI_DROPDOWN_BUTTON_DEFAULT; v <= CUI_DROPDOWN_BUTTON_CUSTOM; v++)
            for (unsigned outline = 0; outline < 2; outline++) {
                o = (cui_dropdown_button_options){.variant = (cui_dropdown_button_variant)v,
                                                  .outline = outline != 0,
                                                  .custom = {{12, 34, 56, 200},
                                                             {21, 43, 65, 210},
                                                             {32, 54, 76, 220},
                                                             {255, 250, 240, 255}}};
                cui_theme_button_kind k = cui_dropdown_kind(o.variant);
                t.buttons[k].normal = (cui_color){11, 22, 33, 120};
                t.buttons[k].border = (cui_color){44, 55, 66, 180};
                b = cui_dropdown_style_box(&t, o, false, true, false, &fg, &m, true, true, false,
                                           true);
                CHECK(eq(fg, alpha(t.muted, .5f)));
                bool outlined = outline || o.variant == CUI_DROPDOWN_BUTTON_OUTLINE;
                if (o.variant == CUI_DROPDOWN_BUTTON_GHOST ||
                    o.variant == CUI_DROPDOWN_BUTTON_LINK || o.variant == CUI_DROPDOWN_BUTTON_TEXT)
                    CHECK(!b.background.a && !b.border.a);
                else if (o.variant == CUI_DROPDOWN_BUTTON_CUSTOM) {
                    CHECK(eq(b.background, alpha(o.custom.color, outlined ? .1f : .15f)));
                    CHECK(eq(b.border, alpha(o.custom.color, outlined ? .2f : .15f)));
                } else if (outlined) {
                    CHECK(eq(b.background, alpha(t.buttons[k].outline_normal, .5f)));
                    CHECK(eq(b.border, alpha(t.buttons[k].outline_border, .5f)));
                } else if (k == CUI_THEME_BUTTON_DEFAULT) {
                    CHECK(eq(b.background, alpha(t.input_background, .5f)) &&
                          eq(b.border, alpha(t.input, .5f)));
                } else {
                    cui_color expected =
                        alpha(t.buttons[k].normal, k == CUI_THEME_BUTTON_SECONDARY ? 1 : .15f);
                    CHECK(eq(b.background, expected) && eq(b.border, expected));
                }
            }
    }
}
typedef struct {
    unsigned count, phases[16];
    cui_button_style_info info[16];
    bool invalid;
} trace;
static void layer(void *user, const cui_button_style_info *info,
                  const cui_button_style_paint *current, cui_button_paint_patch *out) {
    trace *log = user;
    CHECK(log->count < 16);
    log->phases[log->count] = info->phase;
    log->info[log->count++] = *info;
    if (info->phase == CUI_BUTTON_STYLE_INSTANCE) {
        out->fields = CUI_BUTTON_PAINT_BACKGROUND | CUI_BUTTON_PAINT_OPACITY;
        out->paint.root.background = (cui_color){200, 10, 30, 255};
        out->paint.root.opacity = .5f;
        out->paint.root.width = 9999; /* ignored geometry */
        if (log->invalid) {
            out->fields |= CUI_BUTTON_PAINT_RADIUS;
            out->paint.root.radius = NAN;
        }
    } else {
        CHECK(current);
        out->fields = CUI_BUTTON_PAINT_BACKGROUND;
        out->paint.root.background = (cui_color){0, 200, 0, 255};
    }
}
static void callbacks(void) {
    cui_context *ui = make();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    trace log = {0};
    cui_dropdown_button_styles s = {.paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
                                    .action = {.instance = layer,
                                               .selected = layer,
                                               .disabled = layer,
                                               .hovered = layer,
                                               .active = layer,
                                               .focused = layer,
                                               .user = &log}};
    cui_dropdown_button_options o = {.width = 130, .selected = true};
    frame(ui, &menu, o, &s, false);
    CHECK(log.count == 2 && log.phases[0] == CUI_BUTTON_STYLE_INSTANCE &&
          log.phases[1] == CUI_BUTTON_STYLE_SELECTED);
    cui_theme t = cui_theme_light();
    cui_dropdown_button_metrics m = cui_dropdown_button_size_metrics(o.size, 1);
    CHECK(cui_begin_frame(ui, 360, 180, .016f));
    log.count = 0;
    cui_button_style_paint p =
        cui_dropdown_resolve(ui, 2, &t, o, false, false, false, false, &m, &s);
    CHECK(eq(p.root.background, (cui_color){200, 10, 30, 255}) && p.root.width == 130 &&
          p.root.opacity == .5f);
    o.selected = false;
    o.action_loading = true;
    log.count = 0;
    p = cui_dropdown_resolve(ui, 2, &t, o, false, false, true, false, &m, &s);
    CHECK(log.count == 1 && log.info[0].loading && !log.info[0].disabled &&
          fabsf(p.root.opacity - .4f) < .0001f);
    o.action_loading = false;
    log.count = 0;
    p = cui_dropdown_resolve(ui, 2, &t, o, false, true, false, false, &m, &s);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_DISABLED &&
          eq(p.root.background, (cui_color){200, 10, 30, 255}));
    log.count = 0;
    log.invalid = true;
    p = cui_dropdown_resolve(ui, 2, &t, o, false, false, false, false, &m, &s);
    CHECK(eq(p.root.background, t.buttons[CUI_THEME_BUTTON_DEFAULT].normal) &&
          !p.root.custom_opacity);
    (void)cui_end_frame(ui);
    log.count = 0;
    o.hide_action = true;
    frame(ui, &menu, o, &s, false);
    CHECK(!log.count);
    s.container.fields = CUI_BUTTON_PAINT_FOREGROUND;
    menu.drag_scroll = false;
    o.menu.drag_scroll = true;
    frame(ui, &menu, o, &s, false);
    CHECK(!menu.drag_scroll);
    cui_destroy(ui);
}
static unsigned action_calls, body_calls;
static cui_dropdown_button_styles *mutable_styles;
static void action_event(void *user, const cui_component_event_metadata *event) {
    CHECK(user == &action_calls && event->target == 2);
    action_calls++;
    /* Legacy handler mutation occurs before paint; descriptor was copied. */
    mutable_styles->action.instance = NULL;
}
static void body(cui_context *ui, void *user) {
    CHECK(user == &body_calls);
    body_calls++;
    cui_text(ui, cui_string("Custom body"), 12, (cui_color){240, 240, 240, 255});
}
static cui_dropdown_event_result event_frame(cui_context *ui, cui_menu_tree_state *menu,
                                             cui_dropdown_button_styles *s,
                                             cui_dropdown_button_options o) {
    cui_theme theme = cui_theme_light();
    CHECK(cui_begin_frame(ui, 360, 180, .016f));
    o.content = body;
    o.content_user = &body_calls;
    o.menu.popup.viewport = (cui_rect){0, 0, 360, 180};
    mutable_styles = s;
    cui_dropdown_event_result r = cui_dropdown_button_styled_events(
        ui, 1, 2, 3, &theme, cui_string("Save"), cui_string("More"), entries, 1, &command, menu, o,
        s, (cui_dropdown_event_handlers){.action = {action_event, &action_calls}});
    (void)cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK && r.valid);
    return r;
}
static void click(cui_context *ui, cui_id id, bool touch) {
    cui_rect b = cui_bounds(ui, id);
    CHECK(b.w > 0 && b.h > 0);
    for (unsigned up = 0; up < 2; up++)
        CHECK(cui_queue_event(
            ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                            .pointer_id = touch ? 42 : 0,
                            .x = b.x + 4,
                            .y = b.y + 4}));
}
static void routed_inputs(void) {
    cui_context *ui = make();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    trace action = {0}, trigger = {0};
    cui_dropdown_button_styles s = {
        .paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
        .action = {.instance = layer, .user = &action},
        .trigger = {.instance = layer, .selected = layer, .user = &trigger}};
    cui_dropdown_button_options o = {.width = 130};
    action_calls = body_calls = 0;
    event_frame(ui, &menu, &s, o);
    for (unsigned mode = 0; mode < 5; mode++) {
        action.count = trigger.count = 0;
        s.action.instance = layer;
        if (mode < 2) {
            click(ui, 2, mode == 1);
            if (!mode)
                click(ui, 2, false);
        } else if (mode == 2) {
            cui_focus(ui, 2);
            CHECK(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        } else if (mode == 3) {
            cui_focus(ui, 2);
            CHECK(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
            CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
        } else
            CHECK(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
        unsigned before = action_calls, paints = body_calls;
        cui_dropdown_event_result r = event_frame(ui, &menu, &s, o);
        CHECK(r.action_count == (mode ? 1u : 2u) && action_calls - before == r.action_count);
        CHECK(body_calls == paints + 1 && action.count == 1 && !s.action.instance);
    }
    action.count = trigger.count = 0;
    s.action.instance = layer;
    click(ui, 3, false);
    event_frame(ui, &menu, &s, o);
    CHECK(menu.overlay.open && trigger.count == 2 && trigger.info[1].open &&
          trigger.phases[1] == CUI_BUTTON_STYLE_SELECTED && action.count == 1 &&
          !action.info[0].open);
    cui_destroy(ui);
}
static void shadow_gates(void) {
    cui_context *ui = make();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    cui_dropdown_button_options o = {
        .variant = CUI_DROPDOWN_BUTTON_CUSTOM,
        .width = 130,
        .custom = {{20, 40, 60, 255}, {30, 50, 70, 255}, {40, 60, 80, 255}, {250, 250, 250, 255}}};
    unsigned char without[360 * 180 * 4], with[360 * 180 * 4];
    for (unsigned gates = 0; gates < 4; gates++)
        for (unsigned mode = 0; mode < 3; mode++) {
            cui_dropdown_button_styles s = {.paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
                                            .custom_shadow = (gates & 1) != 0,
                                            .host_shadows_enabled = (gates & 2) != 0};
            o.disabled = mode == 1;
            o.action_loading = mode == 2;
            cui_draw_list d = frame(ui, &menu, o, &s, false);
            size_t shadows = 0;
            for (size_t i = 0; i < d.count; i++)
                if (d.commands[i].type == CUI_DRAW_DROP_SHADOW)
                    shadows++;
            CHECK(shadows == ((gates == 3 && mode != 1) ? 2u : 0u));
            if (!gates && !mode)
                pixels(d, without);
            if (gates == 3 && !mode)
                pixels(d, with);
        }
    /* Actual XS software shadow changes pixels outside the painted halves. */
    CHECK(memcmp(without, with, sizeof with));
    cui_rect b = cui_bounds(ui, 2);
    size_t at = ((size_t)(b.y + b.h + 1) * 360 + (size_t)(b.x + b.w / 2)) * 4;
    CHECK(with[at] < without[at]);
    cui_destroy(ui);
}
static void pointer_paint(void) {
    cui_context *ui = make();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    trace log = {0};
    cui_dropdown_button_styles s = {.paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
                                    .action = {.instance = layer,
                                               .hovered = layer,
                                               .active = layer,
                                               .selected = layer,
                                               .focused = layer,
                                               .user = &log}};
    cui_dropdown_button_options o = {.width = 130};
    frame(ui, &menu, o, &s, false);
    cui_rect b = cui_bounds(ui, 2);
    CHECK(cui_queue_event(ui,
                          (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 5, .y = b.y + 5}));
    log.count = 0;
    frame(ui, &menu, o, &s, false);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_HOVER);
    CHECK(cui_queue_event(ui,
                          (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 5, .y = b.y + 5}));
    log.count = 0;
    frame(ui, &menu, o, &s, false);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_ACTIVE);
    o.selected = true;
    log.count = 0;
    frame(ui, &menu, o, &s, false);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_SELECTED);
    CHECK(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 5, .y = b.y + 5}));
    frame(ui, &menu, o, NULL, false);
    cui_focus(ui, 2);
    log.count = 0;
    frame(ui, &menu, o, &s, false);
    CHECK(log.count == 3 && log.phases[2] == CUI_BUTTON_STYLE_FOCUS && log.info[2].focus_visible);
    cui_destroy(ui);
}
static void source_pixels(void) {
    cui_context *ui = make();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    cui_dropdown_button_styles s = {.paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON};
    cui_dropdown_button_options o = {.width = 130, .variant = CUI_DROPDOWN_BUTTON_GHOST};
    frame(ui, &menu, o, &s, false);
    cui_rect b = cui_bounds(ui, 2);
    CHECK(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 10, .y = b.y + 10}));
    cui_draw_list draw = frame(ui, &menu, o, &s, false);
    cui_theme theme = cui_theme_light();
    unsigned char output[360 * 180 * 4];
    pixels(draw, output);
    size_t at = ((size_t)(b.y + b.h / 2) * 360 + (size_t)(b.x + 10)) * 4;
    CHECK(theme.accent.a == 255 && output[at] == theme.accent.r &&
          output[at + 1] == theme.accent.g && output[at + 2] == theme.accent.b);
    o.selected = true;
    draw = frame(ui, &menu, o, &s, false);
    pixels(draw, output);
    cui_color expected = theme.buttons[CUI_THEME_BUTTON_SECONDARY].active;
    CHECK(expected.a == 255 && output[at] == expected.r && output[at + 1] == expected.g &&
          output[at + 2] == expected.b);
    cui_destroy(ui);
}
int main(void) {
    legacy_zero();
    source_formulas();
    callbacks();
    routed_inputs();
    pointer_paint();
    source_pixels();
    shadow_gates();
    return 0;
}
