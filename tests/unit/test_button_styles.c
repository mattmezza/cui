/* SPDX-License-Identifier: MIT */
#include "cui/cui_button_styles.h"
#include "cui/cui_software.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Remain active under Release/NDEBUG: do not put setup in disabled assertions. */
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static const cui_color red = {210, 20, 30, 255}, blue = {10, 40, 220, 255},
                       green = {20, 190, 60, 255}, purple = {130, 20, 140, 255};
static cui_context *create(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 160});
    CHECK(ui);
    cui_set_reduced_motion(ui, true);
    return ui;
}
static bool color_equal(cui_color a, cui_color b) {
    return !memcmp(&a, &b, sizeof a);
}
static bool rect_equal(cui_rect a, cui_rect b) {
    return !memcmp(&a, &b, sizeof a);
}
static cui_draw_list frame(cui_context *ui, cui_button_options o, cui_button_styles styles,
                           bool styled, bool passive, bool *result) {
    cui_theme theme = cui_theme_light();
    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    if (passive) {
        if (styled)
            cui_button_styled_paint(ui, 10, &theme, cui_string("Save"), o, styles);
        else
            cui_button_paint(ui, 10, &theme, cui_string("Save"), o);
        if (result)
            *result = false;
    } else {
        bool activated = styled
                             ? cui_button_styled_ex(ui, 10, &theme, cui_string("Save"), o, styles)
                             : cui_button_ex(ui, 10, &theme, cui_string("Save"), o);
        if (result)
            *result = activated;
    }
    cui_draw_list draw = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void pointer(cui_context *ui, cui_event_type type, bool inside) {
    cui_rect b = cui_bounds(ui, 10);
    CHECK(cui_queue_event(
        ui, (cui_event){.type = type, .x = inside ? b.x + 6 : 310, .y = inside ? b.y + 6 : 150}));
}
static const cui_draw_command *command(cui_draw_list draw, cui_draw_type type) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == type)
            return &draw.commands[i];
    return NULL;
}
static void expect_color(cui_draw_list draw, cui_draw_type type, cui_color color) {
    const cui_draw_command *c = command(draw, type);
    CHECK(c && color_equal(c->color, color));
}
static void pixels(cui_draw_list draw, unsigned char output[320 * 160 * 4]) {
    cui_draw_command commands[256];
    cui_draw_list copy = {.commands = commands, .width = draw.width, .height = draw.height};
    CHECK(draw.count <= 256);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *software = cui_software_create(320, 160, NULL);
    CHECK(software && cui_software_render(software, &copy, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(software, &w, &h);
    CHECK(w == 320 && h == 160);
    memcpy(output, data, 320 * 160 * 4);
    cui_software_destroy(software);
}
static void compare(cui_draw_list a, cui_draw_list b) {
    CHECK(a.count == b.count);
    for (size_t i = 0; i < a.count; i++) {
        const cui_draw_command *x = &a.commands[i], *y = &b.commands[i];
        CHECK(x->type == y->type && rect_equal(x->bounds, y->bounds));
        CHECK(color_equal(x->color, y->color) && x->radius == y->radius && x->stroke == y->stroke);
        CHECK(x->custom_corners == y->custom_corners &&
              x->custom_border_edges == y->custom_border_edges);
        CHECK(!memcmp(&x->corner_radii, &y->corner_radii, sizeof x->corner_radii));
        CHECK(!memcmp(&x->border_edges, &y->border_edges, sizeof x->border_edges));
        CHECK(x->text.length == y->text.length);
        CHECK(!x->text.length || !memcmp(x->text.data, y->text.data, x->text.length));
    }
    unsigned char pa[320 * 160 * 4], pb[320 * 160 * 4];
    pixels(a, pa);
    pixels(b, pb);
    CHECK(!memcmp(pa, pb, sizeof pa));
}
static void noop(void *user, const cui_button_style_info *info,
                 const cui_button_style_paint *current, cui_button_paint_patch *out) {
    (void)user;
    (void)info;
    (void)current;
    (void)out;
}
static void zero_equivalence(void) {
    for (unsigned noops = 0; noops < 2; noops++) {
        cui_context *a = create(), *b = create();
        cui_button_styles styles = {0};
        if (noops)
            styles = (cui_button_styles){.instance = noop,
                                         .hovered = noop,
                                         .active = noop,
                                         .selected = noop,
                                         .disabled = noop,
                                         .focused = noop};
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_CUSTOM; variant++) {
            for (unsigned outline = 0; outline < 2; outline++) {
                cui_button_options o = {.variant = (cui_button_variant)variant,
                                        .outline = outline != 0,
                                        .width = 140,
                                        .hide_focus_ring = true};
                for (unsigned stage = 0; stage < 6; stage++) {
                    if (stage == 0) {
                        pointer(a, CUI_EVENT_CANCEL, false);
                        pointer(b, CUI_EVENT_CANCEL, false);
                        pointer(a, CUI_EVENT_POINTER_MOVE, false);
                        pointer(b, CUI_EVENT_POINTER_MOVE, false);
                    }
                    if (stage == 1) {
                        pointer(a, CUI_EVENT_POINTER_MOVE, true);
                        pointer(b, CUI_EVENT_POINTER_MOVE, true);
                    }
                    if (stage == 2) {
                        pointer(a, CUI_EVENT_POINTER_DOWN, true);
                        pointer(b, CUI_EVENT_POINTER_DOWN, true);
                    }
                    if (stage == 3) {
                        pointer(a, CUI_EVENT_CANCEL, true);
                        pointer(b, CUI_EVENT_CANCEL, true);
                        o.selected = true;
                    }
                    if (stage == 4) {
                        o.selected = false;
                        o.open = true;
                        o.loading = true;
                    }
                    if (stage == 5) {
                        o.disabled = true;
                        o.selected = true;
                    }
                    bool ra, rb;
                    cui_draw_list da = frame(a, o, (cui_button_styles){0}, false, false, &ra);
                    cui_draw_list db = frame(b, o, styles, true, false, &rb);
                    CHECK(ra == rb && rect_equal(cui_bounds(a, 10), cui_bounds(b, 10)));
                    compare(da, db);
                }
            }
        }
        cui_button_options passive = {.width = 140};
        cui_draw_list pa = frame(a, passive, (cui_button_styles){0}, false, true, NULL);
        cui_draw_list pb = frame(b, passive, styles, true, true, NULL);
        compare(pa, pb);
        size_t count;
        cui_semantics(a, &count);
        CHECK(!count);
        cui_semantics(b, &count);
        CHECK(!count);
        cui_destroy(a);
        cui_destroy(b);
    }
}
typedef struct {
    unsigned phases[16], count;
    cui_button_paint_patch patches[6];
    cui_button_style_paint seen[6];
    cui_button_style_info info[6];
} log_t;
static void style(void *user, const cui_button_style_info *info,
                  const cui_button_style_paint *current, cui_button_paint_patch *out) {
    log_t *log = user;
    CHECK(log->count < 16 && info->id == 10 && !out->fields);
    log->phases[log->count++] = (unsigned)info->phase;
    log->seen[info->phase] = *current;
    log->info[info->phase] = *info;
    *out = log->patches[info->phase];
}
static cui_button_styles all_styles(log_t *log) {
    return (cui_button_styles){.instance = style,
                               .hovered = style,
                               .active = style,
                               .selected = style,
                               .disabled = style,
                               .focused = style,
                               .user = log};
}
static cui_button_paint_patch bg(cui_color c) {
    return (cui_button_paint_patch){.fields = CUI_BUTTON_PAINT_BACKGROUND,
                                    .paint.root.background = c};
}
static void layers(void) {
    cui_context *ui = create();
    cui_button_options o = {.width = 140, .hide_focus_ring = true};
    log_t log = {0};
    cui_button_styles styles = all_styles(&log);
    log.patches[CUI_BUTTON_STYLE_INSTANCE] = bg(red);
    log.patches[CUI_BUTTON_STYLE_HOVER] = bg(blue);
    log.patches[CUI_BUTTON_STYLE_ACTIVE] = bg(green);
    log.patches[CUI_BUTTON_STYLE_SELECTED] = bg(purple);
    log.patches[CUI_BUTTON_STYLE_DISABLED] = bg(blue);
    cui_draw_list d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, red);
    CHECK(log.count == 1 && log.phases[0] == CUI_BUTTON_STYLE_INSTANCE);
    pointer(ui, CUI_EVENT_POINTER_MOVE, true);
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, blue);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_HOVER);
    pointer(ui, CUI_EVENT_POINTER_DOWN, true);
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, green);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_ACTIVE);
    CHECK(log.info[CUI_BUTTON_STYLE_ACTIVE].pressed);
    pointer(ui, CUI_EVENT_CANCEL, true);
    o.selected = o.open = true;
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, red);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_SELECTED);
    CHECK(log.info[CUI_BUTTON_STYLE_SELECTED].selected && log.info[CUI_BUTTON_STYLE_SELECTED].open);
    o.disabled = true;
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, red);
    CHECK(log.count == 3 && log.phases[2] == CUI_BUTTON_STYLE_DISABLED);
    log.patches[CUI_BUTTON_STYLE_INSTANCE].fields = 0;
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    expect_color(d, CUI_DRAW_RECT, blue); /* disabled wins absent instance presence */
    CHECK(log.count == 3);
    o.disabled = false;
    o.loading = true;
    log.count = 0;
    log.patches[CUI_BUTTON_STYLE_INSTANCE] =
        (cui_button_paint_patch){.fields = CUI_BUTTON_PAINT_OPACITY, .paint.root.opacity = .5f};
    d = frame(ui, o, styles, true, false, NULL);
    cui_color faded = purple;
    faded.a = (unsigned char)(255 * .4f);
    expect_color(d, CUI_DRAW_RECT, faded);
    CHECK(log.count == 2);
    o.disabled = true;
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    faded = blue;
    faded.a = (unsigned char)(255 * .5f);
    expect_color(d, CUI_DRAW_RECT, faded); /* no loading fade when disabled */
    o = (cui_button_options){.width = 140};
    log = (log_t){0};
    pointer(ui, CUI_EVENT_POINTER_MOVE, false);
    frame(ui, o, styles, true, false, NULL); /* restore eligible semantic node */
    log.count = 0;
    cui_focus(ui, 10); /* explicit focus is keyboard-visible */
    log.patches[CUI_BUTTON_STYLE_FOCUS] = bg(green);
    log.patches[CUI_BUTTON_STYLE_FOCUS].fields |= CUI_BUTTON_PAINT_BORDER;
    log.patches[CUI_BUTTON_STYLE_FOCUS].paint.root.border = blue;
    d = frame(ui, o, styles, true, false, NULL);
    CHECK(log.count == 2 && log.phases[1] == CUI_BUTTON_STYLE_FOCUS);
    expect_color(d, CUI_DRAW_RECT, green);
    expect_color(d, CUI_DRAW_BORDER, blue);
    CHECK(command(d, CUI_DRAW_BORDER)->stroke == 2);
    o.hide_focus_ring = true;
    log.count = 0;
    frame(ui, o, styles, true, false, NULL);
    CHECK(log.count == 1);
    cui_destroy(ui);
}
static void masks_geometry(void) {
    cui_context *ui = create();
    cui_button_options o = {.width = 140, .hide_focus_ring = true};
    log_t log = {0};
    cui_button_styles styles = {.instance = style, .user = &log};
    cui_draw_list d = frame(ui, o, styles, true, false, NULL);
    cui_rect original = cui_bounds(ui, 10);
    cui_color normal = command(d, CUI_DRAW_RECT)->color;
    log.count = 0;
    log.patches[0] = bg((cui_color){0});
    d = frame(ui, o, styles, true, false, NULL);
    unsigned char data[320 * 160 * 4];
    pixels(d, data);
    CHECK(data[(16 * 320 + 30) * 4] == 0 && data[(16 * 320 + 30) * 4 + 1] == 0);
    log.patches[0] =
        (cui_button_paint_patch){.fields = CUI_BUTTON_PAINT_OPACITY, .paint.root.opacity = 0};
    log.count = 0;
    d = frame(ui, o, styles, true, false, NULL);
    pixels(d, data);
    for (size_t i = 0; i < sizeof data; i += 4)
        CHECK(!data[i] && !data[i + 1] && !data[i + 2]);
    for (unsigned bad = 0; bad < 5; bad++) {
        log.count = 0;
        log.patches[0] = bg(red);
        if (bad == 0)
            log.patches[0].fields |= 1u << 20;
        if (bad == 1) {
            log.patches[0].fields |= CUI_BUTTON_PAINT_RADIUS;
            log.patches[0].paint.root.radius = NAN;
        }
        if (bad == 2) {
            log.patches[0].fields |= CUI_BUTTON_PAINT_OPACITY;
            log.patches[0].paint.root.opacity = 1.1f;
        }
        if (bad == 3) {
            log.patches[0].fields |= CUI_BUTTON_PAINT_CORNERS;
            log.patches[0].paint.root.corner_radii.top_left = -1;
        }
        if (bad == 4) {
            log.patches[0].fields |= CUI_BUTTON_PAINT_OPACITY;
            log.patches[0].paint.root.opacity = -.1f;
        }
        d = frame(ui, o, styles, true, false, NULL);
        expect_color(d, CUI_DRAW_RECT, normal);
    }
    log.count = 0;
    log.patches[0] = bg(red);
    log.patches[0].paint.root.width = 900;
    log.patches[0].paint.root.height = NAN;
    log.patches[0].paint.root.padding = 80;
    log.patches[0].paint.root.border_width = 30;
    log.patches[0].paint.root.inert = true;
    d = frame(ui, o, styles, true, false, NULL);
    CHECK(rect_equal(original, cui_bounds(ui, 10)));
    CHECK(command(d, CUI_DRAW_BORDER)->stroke == 1);
    expect_color(d, CUI_DRAW_RECT, red);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    CHECK(n == 1 && !sem[0].disabled && rect_equal(sem[0].bounds, original));
    log.count = 0;
    log.patches[0] = (cui_button_paint_patch){
        .fields = CUI_BUTTON_PAINT_RADIUS | CUI_BUTTON_PAINT_CORNERS | CUI_BUTTON_PAINT_FOREGROUND,
        .paint = {.root = {.radius = 0, .custom_corners = true, .corner_radii = {0, 0, 0, 0}},
                  .foreground = blue}};
    d = frame(ui, o, styles, true, false, NULL);
    CHECK(command(d, CUI_DRAW_RECT)->custom_corners && command(d, CUI_DRAW_RECT)->radius == 0);
    expect_color(d, CUI_DRAW_TEXT, blue);
    log.count = 0;
    log.patches[0] = (cui_button_paint_patch){.fields = CUI_BUTTON_PAINT_BORDER};
    d = frame(ui, o, styles, true, false, NULL);
    CHECK(!command(d, CUI_DRAW_BORDER) || command(d, CUI_DRAW_BORDER)->color.a == 0);
    /* Core may retain an explicitly transparent border command. */
    log.count = 0;
    frame(ui, o, (cui_button_styles){0}, true, false, NULL);
    CHECK(!log.count); /* no retained callback from a preceding frame */
    cui_destroy(ui);
}
static unsigned slot_calls;
static void slot(cui_context *ui, void *user) {
    (void)user;
    slot_calls++;
    cui_begin_box(ui, 33, (cui_box){.width = 20, .height = 10, .background = {90, 10, 200, 255}});
    cui_end_box(ui);
}
static void slots_and_invalid_continuation(void) {
    cui_context *ui = create();
    cui_button_options o = {.width = 140, .content = slot, .hide_focus_ring = true};
    log_t log = {0};
    cui_button_styles styles = {.instance = style, .selected = style, .user = &log};
    frame(ui, o, styles, true, false, NULL);
    cui_rect root = cui_bounds(ui, 10), child = cui_bounds(ui, 33);
    log.count = 0;
    log.patches[0] = bg(red);
    log.patches[0].paint.root.width = 700;
    log.patches[0].paint.root.padding = 80;
    log.patches[0].paint.root.gap = 100;
    frame(ui, o, styles, true, false, NULL);
    CHECK(slot_calls == 2 && rect_equal(root, cui_bounds(ui, 10)) &&
          rect_equal(child, cui_bounds(ui, 33)));
    o.content = NULL;
    o.selected = true;
    log.count = 0;
    log.patches[0].fields |= 1u << 25;
    log.patches[CUI_BUTTON_STYLE_SELECTED] = bg(blue);
    cui_draw_list d = frame(ui, o, styles, true, false, NULL);
    CHECK(log.count == 2);
    expect_color(d, CUI_DRAW_RECT, blue);
    o.selected = false;
    o.custom_corners = true;
    o.corner_radii = (cui_corner_radii){5, 5, 5, 5};
    log.count = 0;
    log.patches[0] = (cui_button_paint_patch){.fields = CUI_BUTTON_PAINT_CORNERS};
    d = frame(ui, o, styles, true, false, NULL);
    /* Core resolves uniform radii into the draw command's explicit corners. */
    CHECK(command(d, CUI_DRAW_RECT)->corner_radii.top_left == cui_theme_light().radius);
    CHECK(command(d, CUI_DRAW_RECT)->corner_radii.bottom_right == cui_theme_light().radius);
    cui_destroy(ui);
}
typedef struct {
    unsigned legacy, typed, sequence[16], count;
} clicks_t;
static void click(void *user) {
    clicks_t *l = user;
    l->legacy++;
    l->sequence[l->count++] = 1;
}
static void typed(void *user, const cui_button_click_event *event) {
    clicks_t *l = user;
    CHECK(event->target == 10);
    l->typed++;
    l->sequence[l->count++] = 2;
}
static void input_passive(void) {
    cui_context *a = create(), *b = create();
    clicks_t la = {0}, lb = {0};
    cui_button_options oa = {
        .width = 140, .on_click = click, .on_click_event = typed, .action_user = &la};
    cui_button_options ob = oa;
    ob.action_user = &lb;
    cui_button_styles styles = {.instance = noop};
    frame(a, oa, styles, false, false, NULL);
    frame(b, ob, styles, true, false, NULL);
    pointer(a, CUI_EVENT_POINTER_DOWN, true);
    pointer(b, CUI_EVENT_POINTER_DOWN, true);
    frame(a, oa, styles, false, false, NULL);
    frame(b, ob, styles, true, false, NULL);
    pointer(a, CUI_EVENT_POINTER_UP, true);
    pointer(b, CUI_EVENT_POINTER_UP, true);
    bool ra, rb;
    frame(a, oa, styles, false, false, &ra);
    frame(b, ob, styles, true, false, &rb);
    CHECK(ra && rb);
    for (unsigned i = 0; i < 2; i++) {
        CHECK(cui_queue_accessibility_action(a, 10, CUI_ACCESSIBILITY_ACTIVATE));
        CHECK(cui_queue_accessibility_action(b, 10, CUI_ACCESSIBILITY_ACTIVATE));
    }
    frame(a, oa, styles, false, false, &ra);
    frame(b, ob, styles, true, false, &rb);
    CHECK(ra && rb && la.legacy == 3 && lb.legacy == 3 && !memcmp(&la, &lb, sizeof la));
    cui_focus(a, 10);
    cui_focus(b, 10);
    CHECK(cui_queue_event(a, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    CHECK(cui_queue_event(b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    CHECK(cui_queue_event(a, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    CHECK(cui_queue_event(b, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    frame(a, oa, styles, false, false, &ra);
    frame(b, ob, styles, true, false, &rb);
    CHECK(ra == rb && !memcmp(&la, &lb, sizeof la) && la.legacy == 4);
    for (unsigned i = 0; i < la.count; i++)
        CHECK(la.sequence[i] == (i % 2 ? 2u : 1u));
    cui_destroy(a);
    cui_destroy(b);
    b = create();
    CHECK(cui_queue_event(b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(b, ob, styles, true, true, &rb);
    size_t n;
    cui_semantics(b, &n);
    CHECK(n == 0 && !rb && lb.legacy == 4);
    cui_destroy(b);
}
typedef struct {
    cui_button_options options;
    unsigned calls;
} mutable_owner;
static void mutate_descriptor(void *user) {
    mutable_owner *owner = user;
    owner->calls++;
    owner->options.disabled = true;
    owner->options.on_click = NULL;
}
static void mutation_equivalence(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        cui_context *ui = create();
        mutable_owner owner = {0};
        owner.options = (cui_button_options){
            .width = 140, .on_click = mutate_descriptor, .action_user = &owner};
        cui_button_styles styles = {.instance = noop};
        frame(ui, owner.options, styles, styled != 0, false, NULL);
        for (unsigned i = 0; i < 2; i++)
            CHECK(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
        bool activated;
        frame(ui, owner.options, styles, styled != 0, false, &activated);
        CHECK(activated && owner.calls == 2 && owner.options.disabled);
        frame(ui, owner.options, styles, styled != 0, false, &activated);
        CHECK(!activated && owner.calls == 2);
        cui_destroy(ui);
    }
}
int main(void) {
    zero_equivalence();
    layers();
    masks_geometry();
    input_passive();
    slots_and_invalid_continuation();
    mutation_equivalence();
    puts("Button bounded styles: ok");
    return 0;
}
