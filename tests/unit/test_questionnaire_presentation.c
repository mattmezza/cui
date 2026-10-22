/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_design_metrics.h"
#include "../fixtures/questionnaire_progress/renderer.h"
#include "cui/cui_questionnaire_presentation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static cui_context *ui;
static cui_theme theme;
static cui_questionnaire_state state;
static cui_questionnaire_item items[2];
static cui_questionnaire_item_runtime runtime[2];
static unsigned events, children;
static cui_questionnaire_text_view observed;
static cui_id namespace_id;
static cui_size measure(void *u, cui_str text, float size, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){(float)text.length * size * .5f, size};
}
static bool metrics(void *u, float size, cui_text_style style, cui_font_metrics *out) {
    (void)u;
    (void)style;
    *out = (cui_font_metrics){size * .75f, size * .25f, size};
    return true;
}
static void event(const cui_questionnaire_state *s, cui_questionnaire_event e, void *u) {
    (void)s;
    (void)e;
    (void)u;
    events++;
}
static void initialize(void) {
    items[0] = (cui_questionnaire_item){.name = cui_string("name"),
                                        .label = cui_string("Title"),
                                        .description = cui_string("Description"),
                                        .required = true};
    items[1] = (cui_questionnaire_item){.name = cui_string("other"), .disabled = true};
    assert(
        cui_questionnaire_init(&state, items, 2,
                               (cui_questionnaire_workspace){.items = runtime, .item_capacity = 2},
                               event, NULL) == CUI_QUESTIONNAIRE_OK);
}
static void custom(cui_context *context, cui_id id, const cui_questionnaire_text_view *view,
                   void *u) {
    children++;
    observed = *view;
    namespace_id = id;
    if (u)
        ((cui_questionnaire_text_options *)u)->visible = cui_string("mutated descriptor");
    cui_begin_box(context, id, (cui_box){.width = -1});
    cui_text_ex(context, view->text, view->font_size, view->color, view->line_height, view->style);
    cui_end_box(context);
}
static void old_metrics(cui_questionnaire_options o, float *font, float *line) {
    const float f[] = {16, 14, 18, 12}, l[] = {24, 20, 28, 16};
    *font = f[(unsigned)o.size];
    *line = l[(unsigned)o.size];
    if (o.design_size) {
        cui_design_metrics d;
        assert(cui_design_resolve(o.design_size, o.density ? o.density : 1, &d));
        *font = d.font_size;
        *line = d.line_height;
    }
}
/* Independent pre-change heading/error text body, with separate named wrappers.
 * Does not call new resolver or legacy forwarding function. */
static void oracle(unsigned role, cui_questionnaire_options o) {
    float f, l;
    old_metrics(o, &f, &l);
    cui_str text = role == 0 ? items[0].label : items[0].description;
    if (role == 2) {
        text = cui_string("This question is required");
        cui_interact(ui, 31, CUI_ROLE_ALERT, text, false, false, 0);
    }
    cui_begin_box(ui, 31, (cui_box){.width = -1});
    if (role == 0)
        cui_text_ex(ui, text, f + 2, theme.text, l + 4,
                    (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    else if (role == 1)
        cui_text_with_line_height(ui, text, f > 14 ? f - 2 : 12, theme.muted, l);
    else
        cui_text(ui, text,
                 o.design_size                       ? f
                 : o.size == CUI_QUESTIONNAIRE_LARGE ? 16
                                                     : 12,
                 theme.danger);
    cui_end_box(ui);
}
static bool submit(unsigned role, cui_questionnaire_options o, cui_questionnaire_text_options p) {
    if (role == 0)
        return cui_questionnaire_title_ex(ui, 31, &theme, &state, 0, o, p);
    if (role == 1)
        return cui_questionnaire_description_ex(ui, 31, &theme, &state, 0, o, p);
    if (role == 2)
        return cui_questionnaire_error_ex(ui, 31, &theme, &state, 0, o, p);
    return cui_questionnaire_choice_description_ex(ui, 31, &theme, items[0].description, o, p);
}
static cui_draw_list frame(unsigned role, cui_questionnaire_options o,
                           cui_questionnaire_text_options p, bool old) {
    assert(cui_begin_frame(ui, 360, 220, 0));
    if (old)
        oracle(role, o);
    else
        assert(submit(role, o, p));
    return cui_end_frame(ui);
}
static bool color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void zero(void) {
    assert(!cui_questionnaire_validate_item(&state, 0));
    for (unsigned scale = 0; scale < 19; scale++) {
        cui_questionnaire_options o = {0};
        if (scale < 4)
            o.size = (cui_questionnaire_size)scale;
        else {
            o.design_size = (cui_design_size)(scale - 3);
            o.density = 1;
        }
        /* Only actual shared design enum values; resolver rejects gaps. */
        cui_design_metrics d;
        if (o.design_size && !cui_design_resolve(o.design_size, 1, &d))
            continue;
        for (unsigned role = 0; role < 3; role++) {
            cui_draw_list a = frame(role, o, (cui_questionnaire_text_options){0}, true);
            assert(a.count < 128);
            cui_draw_command copy[128];
            memcpy(copy, a.commands, a.count * sizeof *copy);
            size_t count = a.count;
            cui_software *r = cui_software_create(360, 220, (cui_font *)&state);
            assert(r);
            assert(cui_software_render(r, &a, theme.background));
            unsigned char pixels[360 * 220 * 4];
            memcpy(pixels, cui_software_pixels(r, NULL, NULL), sizeof pixels);
            size_t sc = 0;
            const cui_semantic *sem = cui_semantics(ui, &sc);
            assert(sc == (role == 2 ? 1u : 0u));
            if (sc)
                assert(sem[0].id == 31 && sem[0].role == CUI_ROLE_ALERT &&
                       sem[0].name.length == 25);
            cui_draw_list b = frame(role, o, (cui_questionnaire_text_options){0}, false);
            assert(count == b.count);
            for (size_t i = 0; i < count; i++) {
                const cui_draw_command *x = &copy[i], *y = &b.commands[i];
                assert(x->type == y->type && x->font_size == y->font_size &&
                       color(x->color, y->color));
                assert(x->bounds.x == y->bounds.x && x->bounds.y == y->bounds.y &&
                       x->bounds.w == y->bounds.w && x->bounds.h == y->bounds.h);
                assert(x->text.length == y->text.length &&
                       (!x->text.length || !memcmp(x->text.data, y->text.data, x->text.length)));
                assert(x->text_style.weight == y->text_style.weight &&
                       x->text_style.family == y->text_style.family);
            }
            assert(cui_software_render(r, &b, theme.background));
            assert(!memcmp(pixels, cui_software_pixels(r, NULL, NULL), sizeof pixels));
            cui_software_destroy(r);
            sem = cui_semantics(ui, &sc);
            assert(sc == (role == 2 ? 1u : 0u));
            if (sc)
                assert(sem[0].id == 31 && sem[0].role == CUI_ROLE_ALERT &&
                       sem[0].name.length == 25);
        }
    }
}
static const cui_draw_command *text(cui_draw_list list) {
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_TEXT)
            return &list.commands[i];
    return NULL;
}
static void tokens(void) {
    const float tf[] = {18, 16, 20, 14}, tl[] = {28, 24, 28, 20}, sf[] = {14, 12, 16, 12},
                sl[] = {20, 16, 24, 16};
    for (unsigned size = 0; size < 4; size++)
        for (unsigned role = 0; role < 4; role++) {
            cui_draw_list list = frame(
                role, (cui_questionnaire_options){.size = (cui_questionnaire_size)size},
                (cui_questionnaire_text_options){.policy = CUI_QUESTIONNAIRE_TEXT_TOKENS}, false);
            const cui_draw_command *draw = text(list);
            assert(draw);
            assert(draw->font_size == (role == 0 ? tf[size] : sf[size]));
            assert(draw->bounds.h == (role == 0 ? tl[size] : sl[size]));
            assert(draw->text_style.weight ==
                   (role == 0 ? CUI_TEXT_WEIGHT_MEDIUM : CUI_TEXT_WEIGHT_NORMAL));
        }
}
static void content(void) {
    unsigned before = events;
    cui_questionnaire_text_options p = {
        .fields = CUI_QUESTIONNAIRE_TEXT_VISIBLE,
        .visible = cui_string("Replacement"),
        .render_child = custom,
        .text = {.fields = 15,
                 .font_size = 21,
                 .line_height = 34,
                 .color = {17, 99, 181, 255},
                 .style = {.weight = CUI_TEXT_WEIGHT_BOLD, .family = 42}}};
    p.render_user = &p;
    cui_draw_list painted = frame(2, (cui_questionnaire_options){0}, p, false);
    assert(children == 1 && namespace_id && namespace_id != 31 && observed.text.length == 11);
    /* Render the actual custom text path, not just its callback snapshot. */
    cui_software *r = cui_software_create(360, 220, (cui_font *)&state);
    assert(r && cui_software_render(r, &painted, theme.background));
    const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
    size_t ink = 0;
    for (size_t i = 0; i < 360u * 220u * 4u; i += 4)
        ink += pixels[i] == 17 && pixels[i + 1] == 99 && pixels[i + 2] == 181;
    assert(ink > 100);
    cui_software_destroy(r);

    assert(observed.font_size == 21 && observed.line_height == 34 && observed.style.family == 42);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].role == CUI_ROLE_ALERT);
    assert(sem[0].name.length == 25 &&
           !cui_accessibility_action_supported(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
    assert(events == before);
    items[0].description = (cui_str){0};
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(cui_questionnaire_description_ex(ui, 31, &theme, &state, 0,
                                            (cui_questionnaire_options){0},
                                            (cui_questionnaire_text_options){0}));
    assert(cui_end_frame(ui).count == 0);
    p = (cui_questionnaire_text_options){.fields = 1, .visible = {NULL, 0}, .render_child = custom};
    frame(1, (cui_questionnaire_options){0}, p, false);
    assert(children == 2 && observed.text.length == 0);
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(cui_questionnaire_title_ex(ui, 32, &theme, &state, 1, (cui_questionnaire_options){0},
                                      (cui_questionnaire_text_options){0}));
    assert(cui_end_frame(ui).count > 0); /* standalone disabled/inactive schema part */
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(!cui_questionnaire_item_begin(ui, 32, &state, 1, (cui_questionnaire_options){0}));
    assert(
        cui_questionnaire_error_ex(ui, 31, &theme, &state, 1, (cui_questionnaire_options){0}, p));
    assert(cui_end_frame(ui).count == 0 && children == 2);
    items[0].description = cui_string("A long localized description with words that must wrap "
                                      "inside the available width without horizontal overflow");
    cui_draw_list list =
        frame(1, (cui_questionnaire_options){0},
              (cui_questionnaire_text_options){.policy = CUI_QUESTIONNAIRE_TEXT_TOKENS}, false);
    assert(list.count > 1 && cui_bounds(ui, 31).w <= 360 && cui_bounds(ui, 31).h > 20);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].bounds.x + list.commands[i].bounds.w <= 360.01f);
    items[0].description = cui_string("Description");
}
static void inheritance_and_passive(void) {
    cui_questionnaire_options root = {.size = CUI_QUESTIONNAIRE_LARGE};
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(cui_questionnaire_begin(ui, 9, &theme, &state, root));
    assert(submit(0, (cui_questionnaire_options){0}, (cui_questionnaire_text_options){0}));
    cui_questionnaire_end(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(text(list)->font_size == 20);
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(cui_questionnaire_begin(ui, 9, &theme, &state, root));
    assert(submit(0, (cui_questionnaire_options){.custom_size = true},
                  (cui_questionnaire_text_options){0}));
    cui_questionnaire_end(ui);
    list = cui_end_frame(ui);
    assert(text(list)->font_size == 18);
    items[0].description = (cui_str){0};
    list = frame(0, (cui_questionnaire_options){.custom_size = true},
                 (cui_questionnaire_text_options){.policy = CUI_QUESTIONNAIRE_TEXT_TOKENS}, false);
    assert(cui_bounds(ui, 31).h == 44); /*28px title plus16px source item gap*/
    items[0].description = cui_string("Description");
    unsigned before = events;
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(submit(2, (cui_questionnaire_options){0}, (cui_questionnaire_text_options){0}));
    (void)cui_interact(ui, 73, CUI_ROLE_BUTTON, cui_string("Sibling"), false, false, 0);
    cui_begin_box(ui, 73, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 5, .y = 5}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 5, .y = 5}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(!cui_queue_accessibility_action(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(ui);
    assert(cui_queue_accessibility_action(ui, 73, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(submit(2, (cui_questionnaire_options){0}, (cui_questionnaire_text_options){0}));
    for (size_t i = 0; i < 4; i++)
        assert(!cui_event_consumed(ui, i));
    assert(cui_interact(ui, 73, CUI_ROLE_BUTTON, cui_string("Sibling"), false, false, 0).activated);
    cui_begin_box(ui, 73, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(events == before && state.current == 0);
    assert(cui_last_error(ui) == CUI_OK);
}
static void invalid(void) {
    cui_questionnaire_text_options bad[] = {
        {.policy = (cui_questionnaire_text_policy)99},
        {.fields = 2},
        {.fields = 1, .visible = {NULL, 1}},
        {.fields = 1, .visible = {"a\0b", 3}},
        {.text = {.fields = 16}},
        {.text = {.fields = 1, .font_size = NAN}},
        {.text = {.fields = 1, .font_size = 0}},
        {.text = {.fields = 2, .line_height = INFINITY}},
        {.text = {.fields = 8, .style.weight = (cui_text_weight)99}}};
    unsigned calls = children;
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
        for (unsigned role = 0; role < 4; role++) {
            bad[i].render_child = custom;
            assert(cui_begin_frame(ui, 360, 220, 0));
            assert(!submit(role, (cui_questionnaire_options){0}, bad[i]));
            assert(cui_end_frame(ui).count == 0);
            size_t n;
            cui_semantics(ui, &n);
            assert(n == 0 && children == calls);
        }
    frame(0, (cui_questionnaire_options){0},
          (cui_questionnaire_text_options){.text = {.font_size = NAN, .line_height = NAN}}, false);
    assert(cui_begin_frame(ui, 360, 220, 0));
    assert(!cui_questionnaire_title_ex(ui, 0, &theme, &state, 0, (cui_questionnaire_options){0},
                                       (cui_questionnaire_text_options){0}));
    assert(!cui_questionnaire_title_ex(ui, 31, &theme, &state, 99, (cui_questionnaire_options){0},
                                       (cui_questionnaire_text_options){0}));
    assert(cui_end_frame(ui).count == 0);
}
int main(void) {
    ui = cui_create((cui_config){.measure_styled = measure, .font_metrics = metrics});
    assert(ui);
    theme = cui_theme_light();
    initialize();
    zero();
    assert(cui_last_error(ui) == CUI_OK);
    tokens();
    assert(cui_last_error(ui) == CUI_OK);
    content();
    assert(cui_last_error(ui) == CUI_OK);
    invalid();
    assert(cui_last_error(ui) == CUI_OK);
    inheritance_and_passive();
    cui_destroy(ui);
    puts("questionnaire presentation passed");
    return 0;
}
