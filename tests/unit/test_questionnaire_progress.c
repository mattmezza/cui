/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_design_metrics.h"
#include "../fixtures/questionnaire_progress/renderer.h"
#include "cui/cui_questionnaire_observer.h"
#include "cui/cui_questionnaire_progress.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    return (cui_size){
        (float)text.length * (size * .5f + (style.weight == CUI_TEXT_WEIGHT_BOLD ? 2 : 0)), size};
}
static bool metrics(void *user, float size, cui_text_style style, cui_font_metrics *out) {
    (void)user;
    (void)style;
    *out = (cui_font_metrics){size * .75f, size * .25f, size};
    return true;
}
static cui_questionnaire_item items[3];
static cui_questionnaire_item_runtime runtime[3];
static cui_questionnaire_state state;
static cui_context *ui;
static cui_software *renderer;
static cui_theme theme;
static unsigned legacy_events, typed_events, children;
static cui_id child_id;
static cui_questionnaire_progress_view child_view;
static void legacy(const cui_questionnaire_state *s, cui_questionnaire_event e, void *user) {
    (void)s;
    (void)e;
    (void)user;
    legacy_events++;
}
static void typed(void *user, const cui_questionnaire_observation *e) {
    (void)user;
    (void)e;
    typed_events++;
}
static void initialize(void) {
    state = (cui_questionnaire_state){0};
    for (size_t i = 0; i < 3; ++i)
        items[i] = (cui_questionnaire_item){.name = cui_string("item")};
    /* Unique identities are required by the actual model. */
    items[1].name = cui_string("second");
    items[2].name = cui_string("third");
    assert(
        cui_questionnaire_init(&state, items, 3,
                               (cui_questionnaire_workspace){.items = runtime, .item_capacity = 3},
                               legacy, NULL) == CUI_QUESTIONNAIRE_OK);
}
/* Independent pre-change progress body (9e5ec407), with expected resolved
 * options supplied by the test. It never calls the new/legacy forwarding seam. */
static void baseline(cui_questionnaire_options o) {
    size_t position = cui_questionnaire_position(&state), total = cui_questionnaire_total(&state);
    size_t current = position == SIZE_MAX ? 0 : position + 1;
    char label[80];
    int n = snprintf(label, sizeof label, "Question %zu of %zu", current, total);
    assert(n >= 0 && (size_t)n < sizeof label);
    cui_str text = {label, (size_t)n};
    cui_interact(ui, 31, CUI_ROLE_PROGRESSBAR, text, false, false, (float)current);
    cui_set_semantic_details(ui, 31, (cui_semantic_details){.minimum = 0, .maximum = (float)total});
    cui_begin_box(ui, 31, (cui_box){.width = -1});
    float font = o.size == CUI_QUESTIONNAIRE_LARGE ? 14 : 12;
    if (o.design_size) {
        cui_design_metrics d;
        assert(cui_design_resolve(o.design_size, o.density > 0 ? o.density : 1, &d));
        font = d.font_size * .75f;
    }
    cui_text_ex(ui, text, font, theme.muted, 0, (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    cui_end_box(ui);
}
static cui_draw_list frame(bool old, cui_questionnaire_options o,
                           cui_questionnaire_progress_options p, bool root) {
    assert(cui_begin_frame(ui, 360, 120, 0));
    if (root)
        assert(cui_questionnaire_begin(ui, 9, &theme, &state, o));
    if (old)
        baseline(o);
    else
        assert(cui_questionnaire_progress_ex(ui, 31, &theme, &state,
                                             root ? (cui_questionnaire_options){0} : o, p));
    if (root)
        cui_questionnaire_end(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static const cui_semantic *progress(void) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    const cui_semantic *result = NULL;
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == 31) {
            assert(!result);
            result = nodes + i;
        }
    assert(result && result->role == CUI_ROLE_PROGRESSBAR);
    assert(!result->disabled && !result->checked && !result->focused);
    assert(!result->details.focusable && !result->details.pointer_activatable);
    return result;
}
static void text_equal(cui_str s, const char *wanted) {
    assert(s.length == strlen(wanted) && (!s.length || !memcmp(s.data, wanted, s.length)));
}
static bool color_equal(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static bool rect_equal(cui_rect a, cui_rect b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}
static bool style_equal(cui_text_style a, cui_text_style b) {
    return a.weight == b.weight && a.italic == b.italic && a.monospace == b.monospace &&
           a.family == b.family && a.letter_spacing == b.letter_spacing;
}
static uint64_t pixels(cui_draw_list list, unsigned *ink) {
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w = 0, h = 0;
    const unsigned char *bytes = cui_software_pixels(renderer, &w, &h);
    assert(bytes && w == 360 && h == 120);
    uint64_t hash = UINT64_C(1469598103934665603);
    *ink = 0;
    for (size_t i = 0; i < (size_t)w * h * 4; ++i) {
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
        if (i % 4 != 3 && bytes[i])
            ++*ink;
    }
    return hash;
}
/* Compare values, never ABI struct padding or frame-owned pointers. */
static void zero_equivalence(void) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        initialize();
        if (mode == 1)
            assert(cui_questionnaire_disable_item(&state, 1, true) == CUI_QUESTIONNAIRE_OK);
        if (mode == 2)
            for (size_t i = 0; i < 3; ++i)
                assert(cui_questionnaire_disable_item(&state, i, true) == CUI_QUESTIONNAIRE_OK);
        if (mode >= 3) {
            for (size_t i = 0; i < 3; ++i)
                assert(cui_questionnaire_skip(&state));
            assert(cui_questionnaire_submit(&state) && state.complete);
            if (mode == 4)
                assert(cui_questionnaire_reset(&state) && !state.complete);
        }
        for (unsigned size = 0; size <= CUI_DESIGN_9XL + 4; ++size) {
            cui_questionnaire_options o = {.custom_size = true};
            if (size < 4) {
                o.size = (cui_questionnaire_size)size;
                o.custom_size = true;
            } else {
                o.design_size = (cui_design_size)(size - 4);
                o.density = .75f;
            }
            for (unsigned root = 0; root < 2; ++root) {
                cui_draw_list old = frame(true, o, (cui_questionnaire_progress_options){0}, root);
                cui_draw_command saved[64];
                char strings[64][128];
                assert(old.count <= 64);
                for (size_t i = 0; i < old.count; ++i) {
                    saved[i] = old.commands[i];
                    assert(saved[i].text.length < 128);
                    if (saved[i].text.length)
                        memcpy(strings[i], saved[i].text.data, saved[i].text.length);
                }
                const cui_semantic *n = progress();
                cui_semantic sem = *n;
                char name[80];
                assert(n->name.length < sizeof name);
                memcpy(name, n->name.data, n->name.length);
                unsigned old_ink, new_ink;
                uint64_t old_hash = pixels(old, &old_ink);
                cui_draw_list now = frame(false, o, (cui_questionnaire_progress_options){0}, root);
                assert(old.count == now.count);
                for (size_t i = 0; i < now.count; ++i) {
                    const cui_draw_command *a = saved + i, *b = now.commands + i;
                    assert(a->type == b->type && rect_equal(a->bounds, b->bounds));
                    assert(color_equal(a->color, b->color) && a->radius == b->radius &&
                           a->stroke == b->stroke && a->font_size == b->font_size &&
                           style_equal(a->text_style, b->text_style));
                    assert(a->text.length == b->text.length &&
                           (!b->text.length || !memcmp(strings[i], b->text.data, b->text.length)));
                }
                n = progress();
                assert(n->id == sem.id && n->role == sem.role && rect_equal(n->bounds, sem.bounds));
                assert(n->disabled == sem.disabled && n->checked == sem.checked &&
                       n->focused == sem.focused && n->value == sem.value &&
                       n->details.minimum == sem.details.minimum &&
                       n->details.maximum == sem.details.maximum &&
                       n->details.focusable == sem.details.focusable &&
                       n->details.pointer_activatable == sem.details.pointer_activatable &&
                       n->tab_index == sem.tab_index && n->tab_ordered == sem.tab_ordered &&
                       n->tab_stop_disabled == sem.tab_stop_disabled &&
                       n->tab_path_count == sem.tab_path_count);
                assert(n->name.length == sem.name.length &&
                       !memcmp(n->name.data, name, n->name.length));
                assert(old_hash == pixels(now, &new_ink) && old_ink == new_ink);
            }
        }
    }
}
static void child(cui_context *context, cui_id id, const cui_questionnaire_progress_view *view,
                  void *user) {
    assert(user == &children && id && id != 31);
    children++;
    if (child_id)
        assert(child_id == id);
    child_id = id;
    child_view = *view;
    cui_begin_box(context, id, (cui_box){.width = 20, .height = 10, .background = view->color});
    cui_end_box(context);
}
static void content_and_metrics(void) {
    initialize();
    cui_questionnaire_progress_options p = {.fields = CUI_QUESTIONNAIRE_PROGRESS_LABEL,
                                            .label = cui_string("Fragé 1 von 3")};
    cui_draw_list list = frame(false, (cui_questionnaire_options){0}, p, false);
    text_equal(progress()->name, "Fragé 1 von 3");
    assert(progress()->value == 1 && progress()->details.minimum == 0 &&
           progress()->details.maximum == 3);
    assert(list.count == 1);
    text_equal(list.commands[0].text, "Fragé 1 von 3");
    p.fields |= CUI_QUESTIONNAIRE_PROGRESS_VISIBLE;
    p.visible = cui_string("1 / 3");
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    text_equal(progress()->name, "Fragé 1 von 3");
    text_equal(list.commands[0].text, "1 / 3");
    p.fields = CUI_QUESTIONNAIRE_PROGRESS_VISIBLE;
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    text_equal(progress()->name, "Question 1 of 3");
    unsigned ink;
    uint64_t normal = pixels(list, &ink);
    assert(ink > 0);
    p.text = (cui_questionnaire_text_refinement){
        .fields = 15,
        .font_size = 20,
        .line_height = 36,
        .color = {255, 0, 0, 255},
        .style = {.weight = CUI_TEXT_WEIGHT_BOLD, .family = 7, .italic = true, .monospace = true}};
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    assert(list.commands[0].font_size == 20 && list.commands[0].bounds.h == 36 &&
           list.commands[0].bounds.y == 8);
    assert(style_equal(list.commands[0].text_style, p.text.style));
    assert(cui_bounds(ui, 31).h == 36);
    assert(color_equal(list.commands[0].color, p.text.color));
    assert(pixels(list, &ink) != normal && ink > 0);
    unsigned w = 0, h = 0;
    const unsigned char *bytes = cui_software_pixels(renderer, &w, &h);
    bool red = false;
    for (size_t i = 0; i < (size_t)w * h; i++)
        if (bytes[4 * i]) {
            assert(bytes[4 * i + 1] == 0 && bytes[4 * i + 2] == 0);
            red = true;
        }
    assert(red);
    p.text.style = (cui_text_style){0};
    p.text.line_height = 0;
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    assert(list.commands[0].text_style.weight == CUI_TEXT_WEIGHT_NORMAL &&
           cui_bounds(ui, 31).h == 20);
    p.text.color.a = 0;
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    (void)pixels(list, &ink);
    assert(!ink);
    p.visible = (cui_str){0};
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    assert(!list.count);
    text_equal(progress()->name, "Question 1 of 3");
    p.fields |= CUI_QUESTIONNAIRE_PROGRESS_LABEL;
    p.label = (cui_str){0};
    frame(false, (cui_questionnaire_options){0}, p, false);
    assert(!progress()->name.length);
    p.label = cui_string("Accessible child");
    p.visible = cui_string("REPLACED visible");
    p.render_child = child;
    p.render_user = &children;
    p.text.color = (cui_color){0, 255, 0, 255};
    list = frame(false, (cui_questionnaire_options){0}, p, false);
    assert(children == 1 && child_view.current == 1 && child_view.total == 3 && list.count == 1);
    assert(list.commands[0].type == CUI_DRAW_RECT);
    (void)pixels(list, &ink);
    assert(ink == 200);
    text_equal(progress()->name, "Accessible child");
    p.label = (cui_str){0};
    p.visible = (cui_str){0};
    frame(false, (cui_questionnaire_options){0}, p, false);
    assert(children == 2 && !progress()->name.length);
    p.text.font_size = 512;
    p.text.line_height = 4096;
    frame(false, (cui_questionnaire_options){0}, p, false);
    assert(child_view.font_size == 512 && child_view.line_height == 4096);
}
static void invalid_preflight(void) {
    cui_questionnaire_progress_options bad[] = {
        {.fields = 4},
        {.text = {.fields = 16}},
        {.fields = 1, .label = {"a\0b", 3}},
        {.fields = 2, .visible = {"\xff", 1}},
        {.fields = 1, .label = {NULL, 1}},
        {.text = {.fields = 1, .font_size = NAN}},
        {.text = {.fields = 1, .font_size = 0}},
        {.text = {.fields = 1, .font_size = 513}},
        {.text = {.fields = 2, .line_height = INFINITY}},
        {.text = {.fields = 2, .line_height = -1}},
        {.text = {.fields = 2, .line_height = 4097}},
        {.text = {.fields = 8, .style = {.weight = (cui_text_weight)99}}},
        {.text = {.fields = 8, .style = {.letter_spacing = NAN}}}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        bad[i].render_child = child;
        unsigned before = children;
        assert(cui_begin_frame(ui, 360, 120, 0));
        assert(!cui_questionnaire_progress_ex(ui, 31, &theme, &state,
                                              (cui_questionnaire_options){0}, bad[i]));
        assert(!cui_end_frame(ui).count && children == before);
        size_t n;
        (void)cui_semantics(ui, &n);
        assert(!n && cui_last_error(ui) == CUI_OK);
    }
    /* Invalid inactive storage is never consulted. */
    frame(false, (cui_questionnaire_options){0},
          (cui_questionnaire_progress_options){.label = {NULL, 10},
                                               .visible = {"\xff", 1},
                                               .text = {.font_size = NAN,
                                                        .line_height = NAN,
                                                        .style = {.weight = (cui_text_weight)99}}},
          false);
    assert(!cui_questionnaire_progress_ex(NULL, 31, &theme, &state, (cui_questionnaire_options){0},
                                          (cui_questionnaire_progress_options){0}));
}
static void passive(void) {
    cui_questionnaire_observer observer = {0};
    assert(cui_questionnaire_observer_attach(&state, &observer, 72, NULL, 0,
                                             (cui_questionnaire_observation_handler){
                                                 typed, NULL}) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    unsigned l = legacy_events, t = typed_events;
    assert(cui_begin_frame(ui, 360, 120, 0));
    assert(cui_questionnaire_progress_ex(ui, 31, &theme, &state, (cui_questionnaire_options){0},
                                         (cui_questionnaire_progress_options){0}));
    (void)cui_interact(ui, 73, CUI_ROLE_BUTTON, cui_string("Sibling"), false, false, 0);
    cui_begin_box(ui, 73, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 5, .y = 5}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 5, .y = 5}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(!cui_accessibility_action_supported(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
    assert(!cui_queue_accessibility_action(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(ui, 73, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_begin_frame(ui, 360, 120, 0));
    assert(cui_questionnaire_progress_ex(ui, 31, &theme, &state, (cui_questionnaire_options){0},
                                         (cui_questionnaire_progress_options){0}));
    for (size_t i = 0; i < 4; i++)
        assert(!cui_event_consumed(ui, i));
    assert(cui_interact(ui, 73, CUI_ROLE_BUTTON, cui_string("Sibling"), false, false, 0).activated);
    cui_begin_box(ui, 73, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_focused(ui) != 31 && state.current == 0 && legacy_events == l && typed_events == t);
    assert(cui_questionnaire_observer_detach(&observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
}
int main(void) {
    ui = cui_create((cui_config){.measure_styled = measure, .font_metrics = metrics});
    assert(ui);
    renderer = cui_software_create(360, 120, (cui_font *)&state);
    assert(renderer);
    theme = cui_theme_light();
    zero_equivalence();
    cui_destroy(ui);
    ui = cui_create((cui_config){.measure_styled = measure, .font_metrics = metrics});
    assert(ui);
    content_and_metrics();
    invalid_preflight();
    passive();
    cui_software_destroy(renderer);
    cui_destroy(ui);
    puts("Questionnaire progress passed");
    return 0;
}
