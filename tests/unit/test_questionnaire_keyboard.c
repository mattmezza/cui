/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire_keyboard.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_questionnaire_state state;
    cui_questionnaire_item item;
    cui_questionnaire_choice choices[2];
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected[2];
    unsigned events[4], shortcuts, content;
    char badge;
} model;
static void event(const cui_questionnaire_state *s, cui_questionnaire_event e, void *user) {
    (void)s;
    ((model *)user)->events[e.kind]++;
}
static void init(model *m) {
    memset(m, 0, sizeof *m);
    m->choices[0].value = cui_string("Alpha");
    m->choices[1].value = cui_string("Beta");
    m->item = (cui_questionnaire_item){
        .name = cui_string("question"), .choices = m->choices, .choice_count = 2};
    assert(cui_questionnaire_init(&m->state, &m->item, 1,
                                  (cui_questionnaire_workspace){.items = &m->runtime,
                                                                .item_capacity = 1,
                                                                .choices = m->selected,
                                                                .choice_capacity = 2},
                                  event, m) == CUI_QUESTIONNAIRE_OK);
}
static void shortcut(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *v,
                     void *user) {
    (void)ui;
    (void)id;
    model *m = user;
    m->shortcuts++;
    m->badge = v->shortcut;
}
static void content(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *v,
                    void *user) {
    (void)ui;
    (void)id;
    model *m = user;
    m->content++;
    m->badge = v->shortcut;
}
static cui_draw_list frame(cui_context *ui, model *m, const cui_questionnaire_keyboard_options *k,
                           bool legacy, bool numeric, bool custom) {
    assert(cui_begin_frame(ui, 320, 240, .001f));
    cui_theme t = cui_theme_light();
    cui_questionnaire_options o = {.width = 300, .numeric_shortcuts = numeric};
    if (legacy)
        assert(cui_questionnaire_begin(ui, 10, &t, &m->state, o));
    else
        assert(cui_questionnaire_begin_with_keyboard(ui, 10, &t, &m->state, o, k));
    assert(cui_questionnaire_item_begin(ui, 11, &m->state, 0, o));
    if (custom) {
        o.render_shortcut = shortcut;
        o.render_content = content;
        o.render_user = m;
    }
    for (size_t c = 0; c < 2; c++)
        (void)cui_questionnaire_choice_control(ui, 12 + c, &t, &m->state, 0, c, o);
    cui_questionnaire_part_end(ui);
    cui_interact(ui, 14, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_begin_box(ui, 14, (cui_box){.width = 40, .height = 30});
    cui_end_box(ui);
    cui_questionnaire_end(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void key(cui_context *ui, cui_key k, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = modifiers}));
}
static void pixels(cui_draw_list draw, unsigned char *out) {
    cui_draw_command commands[256];
    cui_draw_list copy = {.commands = commands, .width = draw.width, .height = draw.height};
    assert(draw.count < 256);
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(320, 240, NULL);
    assert(sw && cui_software_render(sw, &copy, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(sw, &w, &h);
    assert(w == 320 && h == 240);
    memcpy(out, data, 320 * 240 * 4);
    cui_software_destroy(sw);
}
static void policies(void) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        model m;
        init(&m);
        cui_context *ui = cui_create((cui_config){0});
        assert(ui);
        cui_questionnaire_keyboard_options k = {.shortcuts = (cui_questionnaire_shortcut_mode)mode};
        frame(ui, &m, &k, false, false, true);
        assert(m.content == 2 && m.shortcuts == (mode == CUI_QUESTIONNAIRE_SHORTCUT_OFF ? 0u : 2u));
        assert(m.badge == (mode == CUI_QUESTIONNAIRE_SHORTCUT_OFF       ? 0
                           : mode == CUI_QUESTIONNAIRE_SHORTCUT_NUMBERS ? '2'
                                                                        : 'B'));
        cui_focus(ui, 14);
        key(ui, mode == CUI_QUESTIONNAIRE_SHORTCUT_NUMBERS ? CUI_KEY_2 : CUI_KEY_B, 0);
        frame(ui, &m, &k, false, false, true);
        assert(cui_questionnaire_choice_selected(&m.state, 0, 1) ==
               (mode != CUI_QUESTIONNAIRE_SHORTCUT_OFF));
        assert(cui_event_consumed(ui, 0) == (mode != CUI_QUESTIONNAIRE_SHORTCUT_OFF));
        assert(m.events[CUI_QUESTIONNAIRE_ANSWER_CHANGED] ==
               (mode == CUI_QUESTIONNAIRE_SHORTCUT_OFF ? 0u : 1u));
        if (mode == CUI_QUESTIONNAIRE_SHORTCUT_OFF) {
            assert(cui_queue_accessibility_action(ui, 13, CUI_ACCESSIBILITY_ACTIVATE));
            frame(ui, &m, &k, false, false, false);
            assert(cui_questionnaire_choice_selected(&m.state, 0, 1));
            cui_questionnaire_reset(&m.state);
            frame(ui, &m, &k, false, false, false);
            cui_rect r = cui_bounds(ui, 12);
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 5, .y = r.y + 5}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 5, .y = r.y + 5}));
            frame(ui, &m, &k, false, false, false);
            assert(cui_questionnaire_choice_selected(&m.state, 0, 0));
        }
        cui_destroy(ui);
    }
    for (unsigned policy = 0; policy < 2; ++policy) {
        model m;
        init(&m);
        cui_context *ui = cui_create((cui_config){0});
        assert(ui);
        cui_questionnaire_keyboard_options k = {.answer_entry =
                                                    (cui_questionnaire_entry_policy)policy};
        assert(cui_questionnaire_activate(&m.state, 0, 1) == CUI_QUESTIONNAIRE_OK);
        frame(ui, &m, &k, false, false, false);
        cui_focus(ui, 14);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, &m, &k, false, false, false);
        assert(cui_focused(ui) == (policy ? 12u : 13u));
        assert(cui_questionnaire_choice_selected(&m.state, 0, policy ? 0 : 1));
        assert(cui_questionnaire_activate(&m.state, 0, 1) == CUI_QUESTIONNAIRE_OK);
        cui_focus(ui, 11);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, &m, &k, false, false, false);
        assert(cui_focused(ui) == 13);
        cui_destroy(ui);
    }
    for (unsigned policy = 0; policy < 4; ++policy) {
        for (unsigned meta = 0; meta < 2; ++meta) {
            model m;
            init(&m);
            cui_context *ui = cui_create((cui_config){0});
            assert(ui);
            cui_questionnaire_keyboard_options k = {.submit =
                                                        (cui_questionnaire_submit_policy)policy,
                                                    .shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF};
            assert(cui_questionnaire_activate(&m.state, 0, 0) == CUI_QUESTIONNAIRE_OK);
            frame(ui, &m, &k, false, false, false);
            cui_focus(ui, 14);
            key(ui, CUI_KEY_ENTER, meta ? CUI_MOD_META : CUI_MOD_CTRL);
            frame(ui, &m, &k, false, false, false);
            bool expected = policy == 0 || (policy == 2 && !meta) || (policy == 3 && meta);
            assert(m.state.complete == expected);
            assert(m.events[CUI_QUESTIONNAIRE_SUBMIT] == (unsigned)expected);
            cui_destroy(ui);
        }
    }
}
static void equivalence_and_badge(void) {
    static unsigned char a[320 * 240 * 4], b[320 * 240 * 4];
    model x, y;
    init(&x);
    init(&y);
    cui_context *u = cui_create((cui_config){0}), *v = cui_create((cui_config){0});
    assert(u && v);
    cui_questionnaire_keyboard_options zero = {0};
    pixels(frame(u, &x, NULL, true, true, false), a);
    pixels(frame(v, &y, &zero, false, true, false), b);
    assert(!memcmp(a, b, sizeof a));
    size_t uc, vc;
    const cui_semantic *us = cui_semantics(u, &uc), *vs = cui_semantics(v, &vc);
    assert(uc == vc);
    for (size_t i = 0; i < uc; ++i) {
        assert(us[i].id == vs[i].id && us[i].role == vs[i].role && us[i].checked == vs[i].checked &&
               us[i].disabled == vs[i].disabled && us[i].name.length == vs[i].name.length);
        if (us[i].name.length)
            assert(!memcmp(us[i].name.data, vs[i].name.data, us[i].name.length));
    }
    cui_focus(u, 14);
    cui_focus(v, 14);
    key(u, CUI_KEY_2, 0);
    key(v, CUI_KEY_2, 0);
    frame(u, &x, NULL, true, true, false);
    frame(v, &y, &zero, false, true, false);
    assert(!memcmp(x.events, y.events, sizeof x.events));
    assert(cui_focused(u) == cui_focused(v));
    cui_questionnaire_keyboard_options off = {.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF};
    pixels(frame(v, &y, &off, false, false, false), b);
    pixels(frame(v, &y, &zero, false, false, false), a);
    assert(memcmp(a, b, sizeof a)); /* Real badge box pixels disappear. */
    size_t count;
    const cui_semantic *sem = cui_semantics(v, &count);
    bool found = false;
    for (size_t i = 0; i < count; ++i)
        if (sem[i].id == 12) {
            found = true;
            assert(sem[i].role == CUI_ROLE_RADIO && sem[i].name.length == 5);
        }
    assert(found);
    cui_destroy(u);
    cui_destroy(v);
}
static cui_draw_list mismatch_frame(cui_context *ui, model *m, bool extension, bool root_numbers,
                                    bool choice_numbers, bool custom) {
    assert(cui_begin_frame(ui, 320, 240, .001f));
    cui_theme t = cui_theme_light();
    cui_questionnaire_options root = {.width = 300, .numeric_shortcuts = root_numbers};
    const cui_questionnaire_keyboard_options zero = {0};
    if (extension)
        assert(cui_questionnaire_begin_with_keyboard(ui, 10, &t, &m->state, root, &zero));
    else
        assert(cui_questionnaire_begin(ui, 10, &t, &m->state, root));
    cui_questionnaire_options choice = {.numeric_shortcuts = choice_numbers};
    if (custom) {
        choice.render_shortcut = shortcut;
        choice.render_user = m;
    }
    for (size_t c = 0; c < 2; ++c)
        (void)cui_questionnaire_choice_control(ui, 12 + c, &t, &m->state, 0, c, choice);
    cui_questionnaire_end(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void legacy_mismatched_shortcut_inheritance(void) {
    static unsigned char baseline[320 * 240 * 4], extended[320 * 240 * 4], matching[320 * 240 * 4];
    for (unsigned numbers = 0; numbers < 2; ++numbers) {
        model a, b, c;
        init(&a);
        init(&b);
        init(&c);
        cui_context *u = cui_create((cui_config){0});
        cui_context *v = cui_create((cui_config){0});
        cui_context *w = cui_create((cui_config){0});
        assert(u && v && w);
        /* Frozen pre-extension resolve_size -> resolve_shortcuts already copied
         * the active root's numeric flag before choice paint. Both mismatches
         * must therefore match a choice explicitly agreeing with the root. */
        pixels(mismatch_frame(u, &a, false, numbers != 0, numbers == 0, false), baseline);
        pixels(mismatch_frame(v, &b, true, numbers != 0, numbers == 0, false), extended);
        pixels(mismatch_frame(w, &c, false, numbers != 0, numbers != 0, false), matching);
        assert(!memcmp(baseline, extended, sizeof baseline));
        assert(!memcmp(baseline, matching, sizeof baseline));
        mismatch_frame(u, &a, false, numbers != 0, numbers == 0, true);
        mismatch_frame(v, &b, true, numbers != 0, numbers == 0, true);
        assert(a.shortcuts == 2 && b.shortcuts == 2);
        assert(a.badge == (numbers ? '2' : 'B') && b.badge == a.badge);
        /* Root-wide activation follows that same inherited mode. */
        cui_focus(u, 12);
        cui_focus(v, 12);
        key(u, numbers ? CUI_KEY_2 : CUI_KEY_B, 0);
        key(v, numbers ? CUI_KEY_2 : CUI_KEY_B, 0);
        mismatch_frame(u, &a, false, numbers != 0, numbers == 0, false);
        mismatch_frame(v, &b, true, numbers != 0, numbers == 0, false);
        assert(cui_questionnaire_choice_selected(&a.state, 0, 1));
        assert(cui_questionnaire_choice_selected(&b.state, 0, 1));
        assert(!memcmp(a.events, b.events, sizeof a.events));
        cui_destroy(u);
        cui_destroy(v);
        cui_destroy(w);
    }
}
static void standalone(void) {
    for (unsigned numbers = 0; numbers < 2; ++numbers) {
        model m;
        init(&m);
        cui_context *ui = cui_create((cui_config){0});
        assert(ui);
        frame(ui, &m, NULL, true, false, false);
        cui_focus(ui, 14);
        key(ui, numbers ? CUI_KEY_2 : CUI_KEY_B, 0);
        assert(cui_begin_frame(ui, 320, 240, .001f));
        assert(cui_begin_focus_scope(ui, 10, false));
        cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Questionnaire"), false, false, 0);
        cui_begin_box(ui, 10, (cui_box){.width = 300});
        cui_id ids[] = {12, 13};
        cui_questionnaire_keyboard_options k = {0};
        assert(cui_questionnaire_shortcut_keys_with_keyboard(ui, 10, &m.state, ids, 2, 0,
                                                             numbers != 0, &k));
        assert(cui_questionnaire_choice_selected(&m.state, 0, 1));
        cui_end_box(ui);
        cui_end_focus_scope(ui);
        cui_end_frame(ui);
        cui_destroy(ui);
    }
}
static void standalone_entry_and_confirm(void) {
    model m;
    init(&m);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    assert(cui_questionnaire_activate(&m.state, 0, 1) == CUI_QUESTIONNAIRE_OK);
    frame(ui, &m, NULL, true, false, false);
    cui_focus(ui, 14);
    key(ui, CUI_KEY_DOWN, 0);
    assert(cui_begin_frame(ui, 320, 240, .001f));
    assert(cui_begin_focus_scope(ui, 10, false));
    cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Questionnaire"), false, false, 0);
    cui_begin_box(ui, 10, (cui_box){.width = 300});
    cui_id ids[] = {12, 13};
    cui_questionnaire_keyboard_options k = {.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF,
                                            .answer_entry = CUI_QUESTIONNAIRE_ENTRY_ITEM_FOCUS_ONLY,
                                            .submit = CUI_QUESTIONNAIRE_SUBMIT_META};
    assert(cui_questionnaire_answer_keys_with_keyboard(ui, 10, &m.state, ids, 2, 0, 11, &k));
    assert(cui_focused(ui) == 12 && cui_questionnaire_choice_selected(&m.state, 0, 0));
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    frame(ui, &m, NULL, true, false, false);
    cui_focus(ui, 14);
    key(ui, CUI_KEY_B, 0);
    key(ui, CUI_KEY_ENTER, CUI_MOD_META);
    assert(cui_begin_frame(ui, 320, 240, .001f));
    assert(cui_begin_focus_scope(ui, 10, false));
    cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Questionnaire"), false, false, 0);
    cui_begin_box(ui, 10, (cui_box){.width = 300});
    assert(!cui_questionnaire_shortcut_keys_with_keyboard(ui, 10, &m.state, ids, 2, 0, false, &k));
    assert(!cui_event_consumed(ui, 0));
    assert(cui_questionnaire_confirm_keys_with_keyboard(ui, 10, &m.state, ids, 2, 0, CUI_MOD_CTRL,
                                                        &k));
    assert(m.state.complete && cui_event_consumed(ui, 1));
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    cui_destroy(ui);
}
static void normalization_and_guards(void) {
    model m;
    init(&m);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_questionnaire_keyboard_options k = {.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF,
                                            .answer_entry = (cui_questionnaire_entry_policy)999,
                                            .submit = (cui_questionnaire_submit_policy)999};
    assert(cui_questionnaire_activate(&m.state, 0, 1) == CUI_QUESTIONNAIRE_OK);
    frame(ui, &m, &k, false, false, true);
    assert(m.shortcuts == 0);
    cui_focus(ui, 14);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, &m, &k, false, false, false);
    assert(cui_focused(ui) == 13); /* Invalid entry alone falls back. */
    cui_focus(ui, 14);
    key(ui, CUI_KEY_ENTER, CUI_MOD_META);
    frame(ui, &m, &k, false, false, false);
    assert(m.state.complete); /* Invalid submit alone falls back. */
    cui_questionnaire_reset(&m.state);
    k.shortcuts = (cui_questionnaire_shortcut_mode)999;
    k.submit = CUI_QUESTIONNAIRE_SUBMIT_CONTROL;
    frame(ui, &m, &k, false, true, false);
    cui_focus(ui, 14);
    key(ui, CUI_KEY_2, 0);
    frame(ui, &m, &k, false, true, false);
    assert(cui_questionnaire_choice_selected(&m.state, 0, 1));
    k.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_LETTERS;
    cui_questionnaire_reset(&m.state);
    const cui_event ignored[] = {
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .repeat = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .prefer_character_input = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .modifiers = CUI_MOD_ALT}};
    for (size_t n = 0; n < sizeof ignored / sizeof ignored[0]; ++n) {
        assert(cui_queue_event(ui, ignored[n]));
        frame(ui, &m, &k, false, false, false);
        assert(!cui_questionnaire_choice_selected(&m.state, 0, 1));
    }
    assert(cui_questionnaire_activate(&m.state, 0, 0) == CUI_QUESTIONNAIRE_OK);
    cui_focus(ui, 14);
    key(ui, CUI_KEY_ENTER, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    frame(ui, &m, &k, false, false, false);
    assert(!m.state.complete);
    cui_focus(ui, 12);
    key(ui, CUI_KEY_ENTER, 0);
    frame(ui, &m, &k, false, false, false);
    assert(m.state.complete); /* Bare filled-answer confirmation remains. */
    cui_destroy(ui);
}
static void nested_frame(cui_context *ui, model *outer, model *inner) {
    assert(cui_begin_frame(ui, 320, 240, .001f));
    cui_theme t = cui_theme_light();
    cui_questionnaire_options o = {.width = 300};
    cui_questionnaire_keyboard_options k = {.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_LETTERS};
    assert(cui_questionnaire_begin_with_keyboard(ui, 100, &t, &outer->state, o, &k));
    for (size_t c = 0; c < 2; ++c)
        (void)cui_questionnaire_choice_control(ui, 102 + c, &t, &outer->state, 0, c, o);
    assert(cui_questionnaire_begin_with_keyboard(ui, 200, &t, &inner->state, o, &k));
    for (size_t c = 0; c < 2; ++c)
        (void)cui_questionnaire_choice_control(ui, 202 + c, &t, &inner->state, 0, c, o);
    cui_questionnaire_end(ui);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void nested_characterization(void) {
    model a, b;
    init(&a);
    init(&b);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    nested_frame(ui, &a, &b);
    cui_focus(ui, 202);
    key(ui, CUI_KEY_B, 0);
    nested_frame(ui, &a, &b);
    /* Characterization: outer capture precedes inner begin and consumes B.
     * This is deliberately not a nearest-root dispatch/parity assertion. */
    assert(cui_questionnaire_choice_selected(&a.state, 0, 1));
    assert(!cui_questionnaire_choice_selected(&b.state, 0, 1));
    assert(cui_event_consumed(ui, 0));
    cui_destroy(ui);
}
int main(void) {
    policies();
    equivalence_and_badge();
    legacy_mismatched_shortcut_inheritance();
    standalone();
    standalone_entry_and_confirm();
    normalization_and_guards();
    nested_characterization();
    puts("Questionnaire keyboard policies passed");
    return 0;
}
