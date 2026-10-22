/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_design_metrics.h"
#include "../fixtures/questionnaire_geometry/legacy_card.h"
#include "../fixtures/questionnaire_progress/renderer.h"
#include "cui/cui_components.h"
#include "cui/cui_questionnaire_geometry.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
static cui_context *ui;
static cui_theme theme;
static cui_questionnaire_state state;
static cui_questionnaire_item item;
static cui_questionnaire_choice choices[2];
static cui_questionnaire_item_runtime runtime;
static cui_questionnaire_choice_runtime selection[2];
static unsigned events, old_calls, new_calls;
static bool reset_on_change;
static cui_questionnaire_geometry_view observed;
static cui_size measure(void *u, cui_str text, float size, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){text.length * size * .5f, size};
}
static bool font_metrics(void *u, float size, cui_text_style style, cui_font_metrics *out) {
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
    if (reset_on_change && e.kind == CUI_QUESTIONNAIRE_ANSWER_CHANGED) {
        reset_on_change = false;
        assert(cui_questionnaire_reset(&state));
    }
}
static void initialize(void) {
    choices[0] = (cui_questionnaire_choice){.value = cui_string("one"),
                                            .label = cui_string("Answer"),
                                            .description = cui_string("Secondary")};
    choices[1] = (cui_questionnaire_choice){.value = cui_string("two")};
    item = (cui_questionnaire_item){
        .name = cui_string("name"), .choices = choices, .choice_count = 2, .multiple = true};
    assert(cui_questionnaire_init(&state, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = selection,
                                                                .choice_capacity = 2},
                                  event, NULL) == CUI_QUESTIONNAIRE_OK);
}
static cui_draw_list frame(cui_questionnaire_options o, const cui_questionnaire_geometry_options *g,
                           bool oracle) {
    assert(cui_begin_frame(ui, 360, 300, 0));
    if (oracle)
        (void)oracle_choice(ui, 31, &theme, &state, 0, 0, o, NULL);
    else
        (void)cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0, o, NULL, g);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static bool color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void zero(void) {
    for (unsigned scale = 0; scale < 18; scale++)
        for (unsigned mode = 0; mode < 7; mode++) {
            cui_questionnaire_options o = {0};
            if (scale < 4)
                o.size = (cui_questionnaire_size)scale;
            else {
                o.design_size = (cui_design_size)(scale - 3);
                o.density = 1;
            }
            if (mode == 1)
                assert(cui_questionnaire_choose(&state, 0, 0, 0, true) == CUI_QUESTIONNAIRE_OK);
            if (mode == 2)
                o.disabled = true;
            if (mode == 3)
                cui_focus(ui, 31);
            else
                cui_focus(ui, 0);
            theme.dark = mode == 4;
            if (mode == 5) {
                item.required = true;
                (void)cui_questionnaire_validate_item(&state, 0);
            }
            if (mode == 6)
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 5, .y = 5}));
            cui_draw_list a = frame(o, NULL, true);
            cui_draw_command copy[256];
            assert(a.count < 256);
            memcpy(copy, a.commands, a.count * sizeof *copy);
            size_t count = a.count;
            cui_software *r = cui_software_create(360, 300, (cui_font *)&state);
            assert(r && cui_software_render(r, &a, theme.background));
            unsigned char pixels[360 * 300 * 4];
            memcpy(pixels, cui_software_pixels(r, NULL, NULL), sizeof pixels);
            size_t n;
            const cui_semantic *sem = cui_semantics(ui, &n);
            assert(n == 1);
            cui_semantic before = *sem;
            cui_questionnaire_geometry_options g = {0};
            cui_draw_list b = frame(o, &g, false);
            assert(b.count == count);
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
            sem = cui_semantics(ui, &n);
            assert(n == 1 && sem->id == before.id && sem->role == before.role &&
                   sem->checked == before.checked && sem->disabled == before.disabled &&
                   sem->name.length == before.name.length);
            assert(sem->focused == before.focused && sem->value == before.value &&
                   sem->tab_index == before.tab_index && sem->tab_ordered == before.tab_ordered &&
                   sem->tab_stop_disabled == before.tab_stop_disabled);
            assert(sem->bounds.x == before.bounds.x && sem->bounds.y == before.bounds.y &&
                   sem->bounds.w == before.bounds.w && sem->bounds.h == before.bounds.h);
            assert(sem->details.invalid == before.details.invalid &&
                   sem->details.required == before.details.required &&
                   sem->details.pointer_activatable == before.details.pointer_activatable &&
                   sem->details.pos_in_set == before.details.pos_in_set &&
                   sem->details.set_size == before.details.set_size &&
                   sem->details.description.length == before.details.description.length);
            assert(cui_software_render(r, &b, theme.background));
            assert(!memcmp(pixels, cui_software_pixels(r, NULL, NULL), sizeof pixels));
            cui_software_destroy(r);
            item.required = false;
            assert(cui_questionnaire_reset(&state));
        }
    theme = cui_theme_light();
}
static void extended(cui_context *ctx, cui_id id, const cui_questionnaire_geometry_view *view,
                     void *u) {
    new_calls++;
    observed = *view;
    if (u)
        ((cui_questionnaire_geometry_options *)u)->secondary.line_height = 99;
    cui_begin_box(ctx, id, (cui_box){.height = 12, .width = 20});
    cui_end_box(ctx);
}
static void old_child(cui_context *ctx, cui_id id, const cui_questionnaire_choice_view *view,
                      void *u) {
    (void)view;
    (void)u;
    old_calls++;
    cui_begin_box(ctx, id, (cui_box){.height = 12, .width = 20});
    cui_end_box(ctx);
}
static void resolution(void) {
    cui_questionnaire_geometry_options g = {.custom_scalar = 20};
    cui_questionnaire_geometry_metrics m;
    assert(cui_questionnaire_geometry_resolve(&theme, (cui_questionnaire_options){0}, &g, &m));
    assert(m.root_gap == 20 && m.item_gap == 20 && m.choices_gap == 10 && m.answer_gap == 12.5f &&
           m.content_gap == 2.5f);
    assert(m.padding_x == 15 && m.padding_y == 12.5f && m.minimum_height == 55 &&
           m.indicator_size == 20 && m.mark_size == 10 && m.check_size == 17.5f);
    assert(m.shortcut_size == 25 && m.shortcut_font == 12.5f && m.answer_font == 20 &&
           m.answer_line == 30 && m.secondary_font == 17.5f && m.secondary_line == 0);
    assert(m.title_font == 22.5f && m.progress_font == 15 && m.card_radius == theme.radius_lg &&
           m.shortcut_radius == theme.radius);
    for (unsigned i = CUI_DESIGN_XS; i <= CUI_DESIGN_9XL; i++) {
        cui_questionnaire_geometry_metrics a, b;
        cui_questionnaire_options o = {.design_size = (cui_design_size)i, .density = 1};
        assert(cui_questionnaire_geometry_resolve(&theme, o, &g, &a));
        assert(cui_questionnaire_geometry_resolve(&theme, o, NULL, &b));
        assert(a.answer_font == b.answer_font && a.minimum_height == b.minimum_height);
    }
    g.metric_fields =
        CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT | CUI_QUESTIONNAIRE_METRIC_CONTENT_GAP;
    g.metrics.minimum_height = 0;
    g.metrics.content_gap = 0;
    assert(cui_questionnaire_geometry_resolve(&theme, (cui_questionnaire_options){0}, &g, &m) &&
           m.minimum_height == 0 && m.content_gap == 0);
    cui_questionnaire_geometry_metrics saved = m;
    g.custom_scalar = NAN;
    assert(!cui_questionnaire_geometry_resolve(&theme, (cui_questionnaire_options){0}, &g, &m));
    assert(m.minimum_height == saved.minimum_height && m.answer_font == saved.answer_font);
    g.custom_scalar = 256;
    assert(cui_questionnaire_geometry_resolve(&theme, (cui_questionnaire_options){0}, &g, &m));
    g.custom_scalar = 256.01f;
    assert(!cui_questionnaire_geometry_resolve(&theme, (cui_questionnaire_options){0}, &g, &m));
}
static cui_rect bounds(void) {
    return cui_bounds(ui, 31);
}
static void geometry(void) {
    cui_questionnaire_geometry_options g = {
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT,
        .metrics = {.minimum_height = 80},
        .card = {.fields = CUI_QUESTIONNAIRE_BOX_PADDING | CUI_QUESTIONNAIRE_BOX_BACKGROUND |
                           CUI_QUESTIONNAIRE_BOX_GAP,
                 .padding = {.left = 7, .right = 11, .top = 3, .bottom = 5},
                 .background = {201, 29, 47, 255},
                 .gap = 9},
        .render_indicator = extended,
        .render_content = extended,
        .render_shortcut = extended};
    cui_questionnaire_options o = {
        .render_content = old_child, .render_indicator = old_child, .render_shortcut = old_child};
    cui_draw_list list = frame(o, &g, false);
    cui_rect b = bounds();
    assert(b.w == 360 && b.h == 80 && new_calls == 3 && old_calls == 0);
    cui_rect indicator = cui_bounds(ui, 31 ^ UINT64_C(0x751ec681a5920310));
    cui_rect content = cui_bounds(ui, 31 ^ UINT64_C(0x1f0e69b892345708));
    cui_rect badge = cui_bounds(ui, 31 ^ UINT64_C(0x931ac84625c0a917));
    assert(indicator.x == 7 && content.x == indicator.x + 20 + 9 && badge.x + 20 == 360 - 11);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem->id == 31 && sem->bounds.h == 80);
    cui_software *r = cui_software_create(360, 300, (cui_font *)&state);
    assert(r && cui_software_render(r, &list, theme.background));
    const unsigned char *px = cui_software_pixels(r, NULL, NULL);
    assert(px[(60 * 360 + 100) * 4] == 201);
    cui_software_destroy(r);
    unsigned before = events;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = 100,
                                           .y = 60,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = 100,
                                           .y = 60,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    frame(o, &g, false);
    assert(events > before && cui_questionnaire_choice_selected(&state, 0, 0) &&
           cui_focused(ui) == 31);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    frame(o, &g, false);
    assert(!cui_questionnaire_choice_selected(&state, 0, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 31,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    frame(o, &g, false);
    assert(cui_questionnaire_choice_selected(&state, 0, 0));
    o.disabled = true;
    before = events;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 31,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    frame(o, &g, false);
    assert(events == before);
    o.disabled = false;
    g.card_fields = CUI_QUESTIONNAIRE_CARD_FIXED_HEIGHT;
    g.card_height = 79;
    before = events;
    unsigned callbacks = new_calls;
    assert(cui_begin_frame(ui, 360, 300, 0));
    assert(!cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0, o, NULL, &g));
    list = cui_end_frame(ui);
    assert(list.count == 0 && events == before && new_calls == callbacks);
    g.card_fields = 0;
    g.render_content = NULL;
    g.render_indicator = NULL;
    g.render_shortcut = NULL;
    o = (cui_questionnaire_options){0};
    choices[0].description =
        cui_string("Long localized description with enough words to wrap into many many lines in a "
                   "narrow card with a large explicit line box.");
    g.card.fields |= CUI_QUESTIONNAIRE_BOX_WIDTH;
    g.card.width = 160;
    g.secondary = (cui_questionnaire_text_refinement){.fields = CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT,
                                                      .line_height = 32};
    frame(o, &g, false);
    b = bounds();
    assert(b.w == 160 && b.h > 80);
    g.render_content = extended;
    g.render_user = &g;
    g.answer = (cui_questionnaire_text_refinement){.fields = 3, .font_size = 22, .line_height = 0};
    frame(o, &g, false);
    assert(observed.answer.font_size == 22 && observed.answer.line_height == 0 &&
           observed.secondary.line_height == 32);
    g.render_content = NULL;
    o.render_content = old_child;
    frame(o, &g, false);
    assert(old_calls == 1);
}

static void metric_bounds(void) {
    cui_questionnaire_geometry_metrics result = {.answer_font = 123};
    cui_questionnaire_options o = {0};
    cui_questionnaire_geometry_options g = {0};
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_ROOT_GAP,
                                             .metrics.root_gap = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_ITEM_GAP,
                                             .metrics.item_gap = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_CHOICES_GAP,
                                             .metrics.choices_gap = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_ANSWER_GAP,
                                             .metrics.answer_gap = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_CONTENT_GAP,
                                             .metrics.content_gap = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_PADDING_X,
                                             .metrics.padding_x = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_PADDING_Y,
                                             .metrics.padding_y = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT, .metrics.minimum_height = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_CARD_RADIUS,
                                             .metrics.card_radius = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_INDICATOR_SIZE, .metrics.indicator_size = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_MARK_SIZE,
                                             .metrics.mark_size = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_CHECK_SIZE,
                                             .metrics.check_size = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields =
                                                 CUI_QUESTIONNAIRE_METRIC_INDICATOR_RADIUS,
                                             .metrics.indicator_radius = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SHORTCUT_SIZE, .metrics.shortcut_size = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SHORTCUT_FONT, .metrics.shortcut_font = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SHORTCUT_LINE, .metrics.shortcut_line = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SHORTCUT_RADIUS, .metrics.shortcut_radius = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_ANSWER_FONT,
                                             .metrics.answer_font = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_ANSWER_LINE,
                                             .metrics.answer_line = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SECONDARY_FONT, .metrics.secondary_font = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_SECONDARY_LINE, .metrics.secondary_line = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_TITLE_FONT,
                                             .metrics.title_font = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){.metric_fields = CUI_QUESTIONNAIRE_METRIC_TITLE_LINE,
                                             .metrics.title_line = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_PROGRESS_FONT, .metrics.progress_font = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
    g = (cui_questionnaire_geometry_options){
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_PROGRESS_LINE, .metrics.progress_line = NAN};
    assert(!cui_questionnaire_geometry_resolve(&theme, o, &g, &result) &&
           result.answer_font == 123);
}
static void containers(void) {
    cui_questionnaire_geometry_options g = {
        .custom_scalar = 20,
        .root = {.fields = CUI_QUESTIONNAIRE_BOX_PADDING | CUI_QUESTIONNAIRE_BOX_WIDTH,
                 .width = 240,
                 .padding = {.left = 3, .right = 7, .top = 5, .bottom = 9}},
        .item = {.fields = CUI_QUESTIONNAIRE_BOX_GAP, .gap = 0},
        .choices = {.fields = CUI_QUESTIONNAIRE_BOX_GAP, .gap = 11},
        .actions = {.fields = CUI_QUESTIONNAIRE_BOX_GAP, .gap = 13}};
    assert(cui_begin_frame(ui, 360, 300, 0));
    assert(cui_questionnaire_begin_ex(ui, 100, &theme, &state, (cui_questionnaire_options){0}, NULL,
                                      &g));
    assert(cui_questionnaire_item_begin_ex(ui, 101, &state, 0, (cui_questionnaire_options){0}, &g));
    assert(
        cui_questionnaire_choices_begin_ex(ui, 102, &state, 0, (cui_questionnaire_options){0}, &g));
    cui_begin_box(ui, 103, (cui_box){.width = 20, .height = 12});
    cui_end_box(ui);
    cui_begin_box(ui, 104, (cui_box){.width = 20, .height = 12});
    cui_end_box(ui);
    cui_questionnaire_part_end(ui);
    assert(cui_questionnaire_actions_begin_ex(ui, 105, &state, (cui_questionnaire_options){0}, &g));
    cui_begin_box(ui, 106, (cui_box){.width = 20, .height = 12});
    cui_end_box(ui);
    cui_begin_box(ui, 107, (cui_box){.width = 20, .height = 12});
    cui_end_box(ui);
    cui_questionnaire_part_end(ui);
    cui_questionnaire_part_end(ui);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    cui_rect root = cui_bounds(ui, 100), a = cui_bounds(ui, 103), b = cui_bounds(ui, 104),
             x = cui_bounds(ui, 106), y = cui_bounds(ui, 107);
    assert(root.w == 240 && root.h == 61 && a.x == 3 && a.y == 5 && b.y == a.y + 12 + 11 &&
           y.x == x.x + 20 + 13 && x.y == b.y + 12);
    /* Numeric opt-ins never publish to another part; only legacy size does. */
    cui_questionnaire_geometry_options child = {.render_content = extended};
    assert(cui_begin_frame(ui, 360, 300, 0));
    assert(cui_questionnaire_begin_ex(ui, 100, &theme, &state, (cui_questionnaire_options){0}, NULL,
                                      &g));
    (void)cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0,
                                              (cui_questionnaire_options){0}, NULL, &child);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(observed.metrics.answer_font == 16 && observed.metrics.minimum_height == 44);
    /* Final validation uses inherited legacy size, not the unsubmitted default. */
    child = (cui_questionnaire_geometry_options){.card_fields = CUI_QUESTIONNAIRE_CARD_FIXED_HEIGHT,
                                                 .card_height = 32,
                                                 .render_content = extended};
    unsigned before = new_calls;
    assert(cui_begin_frame(ui, 360, 300, 0));
    assert(cui_questionnaire_begin(ui, 100, &theme, &state,
                                   (cui_questionnaire_options){.size = CUI_QUESTIONNAIRE_XSMALL}));
    (void)cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0,
                                              (cui_questionnaire_options){0}, NULL, &child);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(new_calls == before + 1 && bounds().h == 32 && observed.metrics.minimum_height == 28);
}
static void clipping(void) {
    cui_questionnaire_geometry_options g = {
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT,
        .metrics.minimum_height = 80,
        .card = {.fields = CUI_QUESTIONNAIRE_BOX_BACKGROUND, .background = {201, 29, 47, 255}},
        .render_content = extended,
        .render_indicator = extended,
        .render_shortcut = extended};
    assert(cui_begin_frame(ui, 360, 300, 0));
    cui_begin_box(ui, 400, (cui_box){.width = 360, .height = 40, .clip = true});
    (void)cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0,
                                              (cui_questionnaire_options){0}, NULL, &g);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(bounds().h == 80);
    cui_software *r = cui_software_create(360, 300, (cui_font *)&state);
    assert(r && cui_software_render(r, &list, theme.background));
    const unsigned char *px = cui_software_pixels(r, NULL, NULL);
    assert(px[(60 * 360 + 100) * 4] == theme.background.r);
    cui_software_destroy(r);
    unsigned before = events;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 100, .y = 60}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 100, .y = 60}));
    assert(cui_begin_frame(ui, 360, 300, 0));
    cui_begin_box(ui, 400, (cui_box){.width = 360, .height = 40, .clip = true});
    assert(!cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0,
                                                (cui_questionnaire_options){0}, NULL, &g));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(events == before);
}
static void invalid(void) {
    cui_questionnaire_geometry_options bad[] = {
        {.root.fields = 256},
        {.item = {.fields = CUI_QUESTIONNAIRE_BOX_WIDTH, .width = NAN}},
        {.choices = {.fields = CUI_QUESTIONNAIRE_BOX_PADDING, .padding.left = -1}},
        {.actions = {.fields = CUI_QUESTIONNAIRE_BOX_OPACITY, .opacity = 1.1f}},
        {.card = {.fields = CUI_QUESTIONNAIRE_BOX_ALIGN, .align_x = (cui_align)99}},
        {.custom_scalar = -1},
        {.metric_fields = 1u << 25},
        {.card_fields = 2},
        {.metric_fields = CUI_QUESTIONNAIRE_METRIC_ANSWER_FONT, .metrics.answer_font = 0},
        {.metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT,
         .metrics.minimum_height = INFINITY},
        {.answer.fields = 16},
        {.secondary = {.fields = 1, .font_size = 513}},
        {.shortcut = {.fields = 8, .style.weight = (cui_text_weight)99}},
        {.card_fields = 1, .card_height = 0}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        unsigned before = events, calls = new_calls;
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                            .accessibility_target = 31,
                                            .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        assert(cui_begin_frame(ui, 360, 300, 0));
        assert(!cui_questionnaire_begin_ex(ui, 100, &theme, &state, (cui_questionnaire_options){0},
                                           NULL, &bad[i]));
        assert(!cui_questionnaire_item_begin_ex(ui, 101, &state, 0, (cui_questionnaire_options){0},
                                                &bad[i]));
        assert(!cui_questionnaire_choices_begin_ex(ui, 102, &state, 0,
                                                   (cui_questionnaire_options){0}, &bad[i]));
        assert(!cui_questionnaire_actions_begin_ex(ui, 103, &state, (cui_questionnaire_options){0},
                                                   &bad[i]));
        assert(!cui_questionnaire_choice_control_ex(ui, 31, &theme, &state, 0, 0,
                                                    (cui_questionnaire_options){0}, NULL, &bad[i]));
        /* Rejected extension has not consumed the queued activation. */
        assert(cui_interact(ui, 31, CUI_ROLE_CHECKBOX, cui_string("sentinel"), false, false, 0)
                   .activated);
        cui_begin_box(ui, 31, (cui_box){.width = 30, .height = 20});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(events == before && new_calls == calls && state.current == 0);
    }
}
static void callback_mutation(void) {
    for (unsigned old = 0; old < 2; old++) {
        assert(cui_questionnaire_reset(&state));
        cui_questionnaire_geometry_options g = {.render_content = extended, .custom_scalar = 20};
        frame((cui_questionnaire_options){0}, &g, old != 0);
        reset_on_change = true;
        assert(cui_queue_accessibility_action(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
        frame((cui_questionnaire_options){0}, &g, old != 0);
        assert(!reset_on_change && !cui_questionnaire_choice_selected(&state, 0, 0) &&
               !state.complete);
        if (!old)
            assert(!observed.legacy.selected);
    }
}
int main(void) {
    ui = cui_create((cui_config){
        .max_elements = 1024, .measure_styled = measure, .font_metrics = font_metrics});
    assert(ui);
    theme = cui_theme_light();
    initialize();
    metric_bounds();
    resolution();
    zero();
    geometry();
    containers();
    clipping();
    invalid();
    callback_mutation();
    cui_destroy(ui);
    puts("Questionnaire geometry: PASS");
    return 0;
}
