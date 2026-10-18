/* SPDX-License-Identifier: MIT */
#include "cui/cui_settings_events.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_settings_state state;
    cui_settings_page_state page_state[2];
    cui_setting_item items[2];
    cui_setting_group groups[2];
    cui_setting_page pages[2];
    cui_settings_options options;
    cui_settings_navigation_change changes[16];
    bool pending[16];
    size_t count, renders, matches, legacy_changes;
    bool legacy, passive, inert, value, dirty_flag;
    cui_draw_list draw;
} fixture;
static void navigation(void *user, const cui_settings_navigation_change *event) {
    fixture *f = user;
    assert(f->count < 16 && !f->renders);
    assert(f->state.selected_page == event->page && f->state.selected_group == event->group);
    assert(event->previous_page != event->page || event->previous_group != event->group);
    assert(event->event.target == 11 && event->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    f->pending[f->count] = f->state.pending_group_scroll;
    f->changes[f->count++] = *event;
    f->dirty_flag = true; /* unrelated owner state; teardown deferred */
}
static void changed(void *user) {
    ((fixture *)user)->legacy_changes++;
}
static bool matches(cui_str query, void *user) {
    fixture *f = user;
    f->matches++;
    return !query.length || (query.length == 4 && !memcmp(query.data, "beta", 4));
}
static void render(cui_context *ui, cui_id id, const cui_theme *theme,
                   cui_settings_render_options options, void *user) {
    fixture *f = user;
    f->renders++;
    (void)theme;
    (void)options;
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = 250});
    cui_end_box(ui);
}
static cui_settings_event_result frame(fixture *f, bool observe) {
    f->renders = f->matches = 0;
    assert(cui_begin_frame(f->ui, 1000, 600, .016f));
    cui_begin_box(f->ui, 10000, (cui_box){.layout_only = f->passive, .inert = f->inert});
    cui_theme theme = cui_theme_light();
    cui_settings_event_result out = {0};
    if (f->legacy)
        out.result = cui_settings(f->ui, 11, &theme, f->pages, 2, &f->state, f->options);
    else
        out = cui_settings_events(
            f->ui, 11, &theme, f->pages, 2, &f->state, f->options,
            (cui_settings_handler){.on_navigation = observe ? navigation : NULL, .user = f});
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK && !out.result.invalid);
    return out;
}
static void init(fixture *f, bool legacy, size_t default_page) {
    memset(f, 0, sizeof *f);
    f->legacy = legacy;
    f->ui = cui_create((cui_config){.max_elements = 512, .max_commands = 1024});
    assert(f->ui);
    assert(cui_settings_state_init(&f->state, f->page_state, 2, NULL));
    f->items[0] = (cui_setting_item){.title = {"Feature", 7},
                                     .kind = CUI_SETTING_SWITCH,
                                     .boolean = &f->value,
                                     .on_change = changed,
                                     .user = f};
    f->items[1] =
        (cui_setting_item){.title = {"Beta", 4}, .render = render, .matches = matches, .user = f};
    f->groups[0] = (cui_setting_group){.title = {"First", 5}, .items = f->items, .count = 1};
    f->groups[1] = (cui_setting_group){.title = {"Second", 6}, .items = f->items + 1, .count = 1};
    f->pages[0] = (cui_setting_page){
        .title = {"General", 7}, .groups = f->groups, .count = 2, .default_open = true};
    f->pages[1] = (cui_setting_page){.title = {"Other", 5}, .groups = f->groups, .count = 1};
    f->options = cui_settings_default();
    f->options.width = 1000;
    f->options.height = 500;
    f->options.default_page = default_page;
    assert(!frame(f, true).navigation_count && !f->count); /* defaults are maintenance */
}
static void done(fixture *f) {
    cui_destroy(f->ui);
}
static void click(fixture *f, size_t p, size_t g) {
    cui_id id = cui_settings_part_id(11, p, g == CUI_SETTINGS_NO_GROUP ? 0 : g, 0,
                                     g == CUI_SETTINGS_NO_GROUP ? 5 : 6);
    cui_rect r = cui_bounds(f->ui, id);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 2, .y = r.y + 2}));
    assert(cui_queue_event(f->ui,
                           (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 2, .y = r.y + 2}));
}
static void key(fixture *f, cui_key k) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void equal(fixture *a, fixture *b, cui_settings_event_result x,
                  cui_settings_event_result y) {
    assert(x.result.selection_changed == y.result.selection_changed &&
           x.result.reset == y.result.reset && x.result.resized == y.result.resized &&
           x.result.scrolled == y.result.scrolled &&
           x.result.visible_pages == y.result.visible_pages &&
           x.result.visible_groups == y.result.visible_groups &&
           x.result.visible_items == y.result.visible_items &&
           x.result.edit_error == y.result.edit_error);
    assert(a->state.selected_page == b->state.selected_page &&
           a->state.selected_group == b->state.selected_group);
    assert(a->state.pending_group_scroll == b->state.pending_group_scroll &&
           a->state.sidebar_width == b->state.sidebar_width && a->value == b->value);
    assert(a->matches == b->matches && a->renders == b->renders &&
           a->legacy_changes == b->legacy_changes);
    assert(cui_focused(a->ui) == cui_focused(b->ui));
    for (size_t p = 0; p < 2; ++p) {
        assert(a->page_state[p].scroll == b->page_state[p].scroll &&
               a->page_state[p].maximum == b->page_state[p].maximum &&
               a->page_state[p].menu.open == b->page_state[p].menu.open);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color) && p->text.length == q->text.length);
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i)
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role && sa[i].checked == sb[i].checked &&
               sa[i].details.selected == sb[i].details.selected &&
               sa[i].details.expanded == sb[i].details.expanded);
}
static cui_settings_event_result paired(fixture *a, fixture *b, bool observe) {
    cui_settings_event_result x = frame(a, false), y = frame(b, observe);
    equal(a, b, x, y);
    return y;
}
static void navigation_events(void) {
    fixture a, b;
    init(&a, true, 1);
    init(&b, false, 1);
    /* Dispatch follows existing Sidebar item order, not global input order. */
    click(&a, 1, CUI_SETTINGS_NO_GROUP);
    click(&b, 1, CUI_SETTINGS_NO_GROUP);
    click(&a, 0, CUI_SETTINGS_NO_GROUP);
    click(&b, 0, CUI_SETTINGS_NO_GROUP);
    cui_settings_event_result r = paired(&a, &b, true);
    assert(r.result.selection_changed && r.navigation_count == 2 && b.count == 2);
    assert(b.changes[0].previous_page == 1 && b.changes[0].page == 0);
    assert(b.changes[1].previous_page == 0 && b.changes[1].page == 1);
    assert(b.changes[0].event.event_index == 3 && b.changes[1].event.event_index == 1);
    assert(b.changes[0].event.activation.target == cui_settings_part_id(11, 0, 0, 0, 5));
    assert(b.changes[1].event.activation.target == cui_settings_part_id(11, 1, 0, 0, 5));
    assert(b.changes[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(!b.pending[0] && !b.pending[1]);
    click(&a, 0, CUI_SETTINGS_NO_GROUP);
    click(&b, 0, CUI_SETTINGS_NO_GROUP);
    r = paired(&a, &b, true);
    assert(r.navigation_count == 1 && b.state.selected_page == 0);
    click(&a, 0, 1);
    click(&b, 0, 1);
    click(&a, 0, 1);
    click(&b, 0, 1);
    r = paired(&a, &b, true);
    assert(r.navigation_count == 1 && b.state.selected_group == 1);
    assert(b.pending[3]); /* Content later resolves pending scroll at its old point. */
    assert(b.changes[3].event.event_index == 1 && b.changes[3].group == 1);
    b.dirty_flag = false;
    click(&a, 0, 1);
    click(&b, 0, 1);
    r = paired(&a, &b, true);
    assert(!r.navigation_count && !r.result.selection_changed && !b.dirty_flag);

    click(&a, 0, 0);
    click(&b, 0, 0);
    click(&a, 0, 1);
    click(&b, 0, 1);
    r = paired(&a, &b, true);
    assert(r.navigation_count == 2 && b.state.selected_group == 1);
    assert(b.changes[4].previous_group == 1 && b.changes[4].group == 0);
    assert(b.changes[5].previous_group == 0 && b.changes[5].group == 1);
    cui_id nav = cui_settings_part_id(11, 1, 0, 0, 5);
    cui_focus(a.ui, nav);
    cui_focus(b.ui, nav);
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    r = paired(&a, &b, true);
    assert(r.navigation_count == 1 && b.state.selected_page == 1);
    assert(b.changes[6].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(b.changes[6].event.event_index == 0 && b.changes[6].event.activation.target == nav);
    click(&a, 0, CUI_SETTINGS_NO_GROUP);
    click(&b, 0, CUI_SETTINGS_NO_GROUP);
    r = paired(&a, &b, false);
    assert(r.result.selection_changed && !r.navigation_count);
    /* Legacy field callbacks remain after navigation, untouched. */
    cui_id field = cui_settings_part_id(11, 0, 0, 0, 24);
    cui_focus(a.ui, field);
    cui_focus(b.ui, field);
    key(&a, CUI_KEY_SPACE);
    key(&b, CUI_KEY_SPACE);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    r = paired(&a, &b, true);
    assert(!r.navigation_count && b.value && b.legacy_changes == 1);
    /* Filtering changes selection/result, never reports user navigation. */
    click(&a, 1, CUI_SETTINGS_NO_GROUP);
    click(&b, 1, CUI_SETTINGS_NO_GROUP);
    paired(&a, &b, false);
    a.options.query = b.options.query = cui_string("beta");
    r = paired(&a, &b, true);
    assert(!r.navigation_count && r.result.selection_changed && r.result.visible_pages == 1);
    assert(b.state.selected_page == 0 && b.state.selected_group == CUI_SETTINGS_NO_GROUP);
    a.options.query = b.options.query = cui_string("missing");
    assert(!paired(&a, &b, true).navigation_count);
    a.options.query = b.options.query = (cui_str){0};
    paired(&a, &b, true);
    cui_focus(a.ui, nav);
    cui_focus(b.ui, nav);
    a.state.selected_group = b.state.selected_group = 1;
    a.options.query = b.options.query = cui_string("Feature");
    r = paired(&a, &b, true);
    assert(r.result.selection_changed && !r.navigation_count &&
           b.state.selected_group == CUI_SETTINGS_NO_GROUP);
    a.options.query = b.options.query = (cui_str){0};
    paired(&a, &b, true);
    a.inert = b.inert = true;
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    assert(!paired(&a, &b, true).navigation_count);
    a.inert = b.inert = false;
    cui_focus(a.ui, nav);
    cui_focus(b.ui, nav);
    a.passive = b.passive = true;
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    assert(!paired(&a, &b, true).navigation_count);
    cui_settings_navigation_change copy = b.changes[0];
    assert(cui_begin_frame(b.ui, 1000, 600, .016f));
    cui_end_frame(b.ui); /* deferred omission */
    done(&a);
    done(&b);
    assert(copy.page == 0 && copy.event.activation.target);
}
static void narrow_and_admission(void) {
    fixture a, b;
    init(&a, true, 0);
    init(&b, false, 0);
    a.options.width = b.options.width = 390;
    paired(&a, &b, true);
    click(&a, 1, CUI_SETTINGS_NO_GROUP);
    click(&b, 1, CUI_SETTINGS_NO_GROUP);
    cui_settings_event_result r = paired(&a, &b, true);
    assert(r.navigation_count == 1 && b.state.selected_page == 1);
    cui_id nav = cui_settings_part_id(11, 1, 0, 0, 5);
    assert(!cui_accessibility_action_supported(b.ui, nav, CUI_ACCESSIBILITY_ACTIVATE));
    cui_settings_state before = b.state;
    cui_theme theme = cui_theme_light();
    r = cui_settings_events(b.ui, 0, &theme, b.pages, 2, &b.state, b.options,
                            (cui_settings_handler){.on_navigation = navigation, .user = &b});
    assert(r.result.invalid && !r.navigation_count && !memcmp(&before, &b.state, sizeof before));
    done(&a);
    done(&b);
}
typedef struct scalar_fixture scalar_fixture;
typedef struct {
    scalar_fixture *f;
    size_t index;
} scalar_user;
struct scalar_fixture {
    cui_context *ui;
    cui_settings_state state;
    cui_settings_page_state workspace[1];
    cui_setting_item items[4];
    cui_setting_group group;
    cui_setting_page page;
    cui_settings_options options;
    bool booleans[2], before_bool[2];
    double number, before_number;
    cui_id choice, before_choice;
    char draft[64];
    cui_edit *edit;
    cui_number_state number_state;
    cui_choice_item choices[3];
    cui_choice_popup popup;
    cui_settings_dropdown_state menu;
    cui_menu_entry entries[3];
    bool checks[3];
    scalar_user users[4];
    unsigned mode[4]; /* 0 accept,1 restore entry,2 redirect,3 poison later Number */
    size_t legacy_calls[4], boolean_calls, number_calls, choice_calls;
    cui_settings_bool_change last_boolean;
    cui_settings_number_change last_number;
    cui_settings_choice_change last_choice;
    cui_settings_text_change last_text;
    cui_input_state input;
    char text_copy[64];
    size_t text_calls;
    bool legacy, passive, inert;
    char order[32];
    size_t order_count;
    cui_draw_list draw;
};
static void scalar_order(scalar_fixture *f, char code) {
    assert(f->order_count < sizeof f->order);
    f->order[f->order_count++] = code;
}
static void scalar_legacy(void *user) {
    scalar_user *u = user;
    scalar_fixture *f = u->f;
    size_t i = u->index;
    ++f->legacy_calls[i];
    scalar_order(f, 'L');
    if (f->items[i].kind == CUI_SETTING_INPUT) {
        if (f->mode[i] == 1)
            assert(cui_edit_replace_range(f->edit, 0, cui_edit_get_state(f->edit).length,
                                          cui_string("")) == CUI_EDIT_OK);
        else if (f->mode[i] == 2)
            assert(cui_edit_replace_range(f->edit, 0, cui_edit_get_state(f->edit).length,
                                          cui_string("redirect")) == CUI_EDIT_OK);
        return;
    }
    if (f->mode[i] == 1) {
        if (i < 2)
            f->booleans[i] = f->before_bool[i];
        else if (i == 2)
            f->number = f->before_number;
        else
            f->choice = f->before_choice;
    } else if (f->mode[i] == 2) {
        if (i == 2)
            f->number = 7.5;
        else if (i == 3)
            f->choice = 102;
    } else if (f->mode[i] == 3)
        f->number = NAN;
    else if (f->mode[i] == 4)
        f->number = INFINITY;
    else if (f->mode[i] == 5)
        f->number = -0.0;
}
static void scalar_custom(cui_context *ui, cui_id id, const cui_theme *theme,
                          cui_settings_render_options options, void *user) {
    (void)ui;
    (void)id;
    (void)theme;
    (void)options;
    scalar_user *u = user;
    u->f->booleans[u->index] = !u->f->booleans[u->index];
}
static void scalar_metadata(cui_component_event_metadata e, cui_id field, size_t p, size_t g,
                            size_t i) {
    assert(e.kind == CUI_COMPONENT_EVENT_CHANGE && e.target == 11);
    assert(e.flags == CUI_COMPONENT_EVENT_HAS_FRAME && e.frame_sequence);
    assert(e.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(!p && !g && field == cui_settings_part_id(11, p, g, i, 24));
}
static void boolean_event(void *user, const cui_settings_bool_change *event) {
    scalar_fixture *f = user;
    assert(event->item < 2);
    scalar_metadata(event->event, event->field_id, event->page, event->group, event->item);
    assert(f->booleans[event->item] == event->value && event->previous != event->value);
    assert(f->legacy_calls[event->item]);
    f->last_boolean = *event;
    f->boolean_calls++;
    scalar_order(f, 'B');
}
static void number_event(void *user, const cui_settings_number_change *event) {
    scalar_fixture *f = user;
    scalar_metadata(event->event, event->field_id, event->page, event->group, event->item);
    assert(event->item == 2 && f->number == event->value && f->legacy_calls[2]);
    assert(event->previous_finite == isfinite(event->previous) &&
           event->value_finite == isfinite(event->value));
    f->last_number = *event;
    f->number_calls++;
    scalar_order(f, 'N');
}
static void choice_event(void *user, const cui_settings_choice_change *event) {
    scalar_fixture *f = user;
    scalar_metadata(event->event, event->field_id, event->page, event->group, event->item);
    assert(event->item == 3 && f->choice == event->value && f->legacy_calls[3]);
    f->last_choice = *event;
    f->choice_calls++;
    scalar_order(f, 'C');
}
static void text_event(void *user, const cui_settings_text_change *e) {
    scalar_fixture *f = user;
    assert(e->reason == CUI_SETTINGS_CHANGE_EDIT);
    assert(e->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(e->before_revision != e->after_revision);
    assert(e->after_revision == cui_edit_get_state(f->edit).revision);
    assert(f->legacy_calls[0]);
    cui_str copied = {0};
    assert(cui_component_event_copy_utf8(e->text, 63, f->text_copy, sizeof f->text_copy, &copied) == CUI_COMPONENT_EVENT_COPY_OK);
    assert(copied.length == e->after_bytes);
    char storage[2] = {'A', 'B'};
    cui_str sentinel = cui_string("unchanged");
    if (e->text.length > 1) {
        assert(cui_component_event_copy_utf8(e->text, 1, storage, sizeof storage, &sentinel)
               == CUI_COMPONENT_EVENT_COPY_LIMIT);
        assert(storage[0] == 'A' && storage[1] == 'B' && sentinel.length == 9);
    }
    f->last_text = *e;
    ++f->text_calls;
}
static cui_settings_event_result scalar_frame(scalar_fixture *f, bool observe) {
    f->before_bool[0] = f->booleans[0];
    f->before_bool[1] = f->booleans[1];
    f->before_number = f->number;
    f->before_choice = f->choice;
    f->order_count = 0;
    assert(cui_begin_frame(f->ui, 1000, 800, .016f));
    cui_begin_box(f->ui, 10000, (cui_box){.layout_only = f->passive, .inert = f->inert});
    cui_theme t = cui_theme_light();
    cui_settings_event_result out = {0};
    if (f->legacy)
        out.result = cui_settings(f->ui, 11, &t, &f->page, 1, &f->state, f->options);
    else
        out =
            cui_settings_events(f->ui, 11, &t, &f->page, 1, &f->state, f->options,
                                (cui_settings_handler){.user = f,
                                                       .on_boolean = observe ? boolean_event : NULL,
                                                       .on_number = observe ? number_event : NULL,
                                                       .on_choice = observe ? choice_event : NULL,
                                                       .on_text = observe ? text_event : NULL});
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return out;
}
static void scalar_init(scalar_fixture *f, bool legacy, bool menu) {
    memset(f, 0, sizeof *f);
    f->legacy = legacy;
    f->number = 3;
    f->choice = 101;
    f->ui = cui_create((cui_config){.max_elements = 512, .max_commands = 1024});
    assert(f->ui);
    assert(cui_settings_state_init(&f->state, f->workspace, 1, NULL));
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->draft, .capacity = sizeof f->draft});
    assert(f->edit);
    f->number_state.edit = f->edit;
    f->choices[0] = (cui_choice_item){101, {"First", 5}, false};
    f->choices[1] = (cui_choice_item){102, {"Second", 6}, false};
    f->choices[2] = (cui_choice_item){103, {"Third", 5}, false};
    cui_choice_popup_init(&f->popup, 9000);
    assert(cui_settings_dropdown_init(&f->menu, 9001, f->entries, f->checks, 3));
    f->items[0] = (cui_setting_item){
        .title = {"Switch", 6}, .kind = CUI_SETTING_SWITCH, .boolean = f->booleans};
    f->items[1] = (cui_setting_item){
        .title = {"Check", 5}, .kind = CUI_SETTING_CHECKBOX, .boolean = f->booleans + 1};
    f->items[2] = (cui_setting_item){
        .title = {"Number", 6},
        .kind = CUI_SETTING_NUMBER,
        .number = &f->number,
        .number_state = &f->number_state,
        .number_options = {.has_minimum = true, .minimum = 0, .has_maximum = true, .maximum = 10}};
    f->items[3] = (cui_setting_item){.title = {"Mode", 4},
                                     .kind = CUI_SETTING_DROPDOWN,
                                     .choice = &f->choice,
                                     .choices = f->choices,
                                     .choice_count = 3,
                                     .popup = &f->popup,
                                     .dropdown_state = menu ? &f->menu : NULL};
    for (size_t i = 0; i < 4; ++i) {
        f->users[i] = (scalar_user){f, i};
        f->items[i].on_change = scalar_legacy;
        f->items[i].user = f->users + i;
    }
    f->group = (cui_setting_group){.items = f->items, .count = 4};
    f->page = (cui_setting_page){.title = {"Settings", 8}, .groups = &f->group, .count = 1};
    f->options = cui_settings_default();
    f->options.width = 1000;
    f->options.height = 700;
    assert(!scalar_frame(f, true).result.invalid);
}
static void scalar_done(scalar_fixture *f) {
    cui_destroy(f->ui);
    cui_edit_destroy(f->edit);
}
static void scalar_key(scalar_fixture *f, size_t item, cui_key key) {
    cui_focus(f->ui, cui_settings_part_id(11, 0, 0, item, 24));
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void scalar_toggle(scalar_fixture *f, size_t item) {
    scalar_key(f, item, CUI_KEY_SPACE);
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
}
static cui_settings_event_result scalar_pair(scalar_fixture *a, scalar_fixture *b, bool observe) {
    cui_settings_event_result x = scalar_frame(a, false), y = scalar_frame(b, observe);
    assert(x.result.invalid == y.result.invalid && x.result.edit_error == y.result.edit_error &&
           x.result.selection_changed == y.result.selection_changed &&
           x.result.reset == y.result.reset);
    assert(a->booleans[0] == b->booleans[0] && a->booleans[1] == b->booleans[1]);
    assert(a->choice == b->choice &&
           (a->number == b->number || (isnan(a->number) && isnan(b->number))));
    assert(a->number_state.last_value == b->number_state.last_value ||
           (isnan(a->number_state.last_value) && isnan(b->number_state.last_value)));
    assert(!memcmp(a->legacy_calls, b->legacy_calls, sizeof a->legacy_calls));
    assert(!strcmp(a->draft, b->draft) && cui_focused(a->ui) == cui_focused(b->ui));
    assert(a->popup.overlay.open == b->popup.overlay.open &&
           a->menu.menu.overlay.open == b->menu.menu.overlay.open);
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color) && p->text.length == q->text.length);
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i)
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role && sa[i].checked == sb[i].checked);
    return y;
}
static void scalar_boolean_and_number(void) {
    scalar_fixture a, b;
    scalar_init(&a, true, true);
    scalar_init(&b, false, true);
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    cui_settings_event_result r = scalar_pair(&a, &b, true);
    assert(r.boolean_count == 1 && b.last_boolean.value && b.order_count == 2 &&
           !memcmp(b.order, "LB", 2));
    a.mode[1] = b.mode[1] = 1;
    scalar_toggle(&a, 1);
    scalar_toggle(&b, 1);
    r = scalar_pair(&a, &b, true);
    assert(!r.boolean_count && !b.booleans[1] && b.legacy_calls[1] == 1);
    a.mode[1] = b.mode[1] = 0;
    scalar_toggle(&a, 1);
    scalar_toggle(&b, 1);
    r = scalar_pair(&a, &b, true);
    assert(r.boolean_count == 1 && b.last_boolean.item == 1);
    /* Legacy Switch coalesces activations: preserve its single applied flip. */
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    assert(scalar_pair(&a, &b, true).boolean_count == 1);
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    assert(!scalar_pair(&a, &b, false).boolean_count);
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    r = scalar_pair(&a, &b, true);
    assert(r.number_count == 1 && b.last_number.previous == 3 && b.last_number.value == 4);
    assert(b.order_count == 2 && !memcmp(b.order, "LN", 2));
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    scalar_key(&a, 2, CUI_KEY_DOWN);
    scalar_key(&b, 2, CUI_KEY_DOWN);
    r = scalar_pair(&a, &b, true);
    assert(!r.number_count && b.number == 4);
    a.mode[2] = b.mode[2] = 1;
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    assert(!scalar_pair(&a, &b, true).number_count && b.number == 4);
    a.mode[2] = b.mode[2] = 2;
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    r = scalar_pair(&a, &b, true);
    assert(r.number_count == 1 && b.last_number.previous == 4 && b.last_number.value == 7.5);
    assert(b.number_state.last_value == a.number_state.last_value &&
           b.number_state.last_value == 5);
    a.mode[2] = b.mode[2] = 0;
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    r = scalar_pair(&a, &b, false);
    assert(!r.number_count && b.number == 8.5);
    /* Extra Settings bounds execute even after nested control argument errors. */
    a.number = b.number = 20;
    a.items[2].number_options.prefix = b.items[2].number_options.prefix = (cui_str){NULL, 1};
    r = scalar_pair(&a, &b, true);
    assert(r.result.edit_error == CUI_EDIT_INVALID_ARGUMENT && r.number_count == 1 &&
           b.number == 10);
    assert(b.number_state.last_value == 10);
    a.items[2].number_options.prefix = b.items[2].number_options.prefix = (cui_str){0};
    a.items[2].disabled = b.items[2].disabled = true;
    a.number = b.number = 20;
    r = scalar_pair(&a, &b, true);
    assert(!r.number_count && b.number == 10);
    a.items[2].disabled = b.items[2].disabled = false;
    a.passive = b.passive = true;
    a.number = b.number = 20;
    assert(!scalar_pair(&a, &b, true).number_count && b.number == 10);
    a.passive = b.passive = false;
    a.inert = b.inert = true;
    a.number = b.number = 20;
    assert(!scalar_pair(&a, &b, true).number_count && b.number == 10);
    a.inert = b.inert = false;
    a.number = b.number = 0;
    a.mode[2] = b.mode[2] = 5;
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    r = scalar_pair(&a, &b, true);
    assert(!r.number_count && signbit(b.number));
    a.mode[2] = b.mode[2] = 0;
    a.number = b.number = 3;
    a.items[0].render = b.items[0].render = scalar_custom;
    r = scalar_pair(&a, &b, true);
    assert(!r.boolean_count);
    a.items[0].render = b.items[0].render = NULL;
    /* Legacy callback may redirect to nonfinite owner after the existing bounds pass. */
    a.mode[2] = b.mode[2] = 4;
    scalar_key(&a, 2, CUI_KEY_UP);
    scalar_key(&b, 2, CUI_KEY_UP);
    r = scalar_pair(&a, &b, true);
    assert(r.number_count == 1 && !b.last_number.value_finite && b.last_number.previous_finite);
    a.number = b.number = 3;
    a.mode[2] = b.mode[2] = 0;
    /* Earlier legacy field callback can poison Number after outer validation. */
    a.mode[0] = b.mode[0] = 3;
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    r = scalar_pair(&a, &b, true);
    assert(!r.number_count && r.result.edit_error == CUI_EDIT_INVALID_ARGUMENT);
    cui_settings_number_change copy = b.last_number;
    a.number = b.number = 3;
    a.mode[2] = b.mode[2] = 2;
    scalar_toggle(&a, 0);
    scalar_toggle(&b, 0);
    r = scalar_pair(&a, &b, true);
    assert(r.number_count == 1 && !b.last_number.previous_finite && b.last_number.value_finite &&
           b.number == 7.5);
    scalar_done(&a);
    scalar_done(&b);
    assert(!copy.value_finite && isinf(copy.value));
}
static void scalar_choices(bool menu) {
    scalar_fixture a, b;
    scalar_init(&a, true, menu);
    scalar_init(&b, false, menu);
    scalar_key(&a, 3, CUI_KEY_DOWN);
    scalar_key(&b, 3, CUI_KEY_DOWN);
    scalar_pair(&a, &b, true);
    assert(menu ? b.menu.menu.overlay.open : b.popup.overlay.open);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    cui_settings_event_result r = scalar_pair(&a, &b, true);
    assert(r.choice_count == 1 && b.choice == 103 && b.last_choice.previous == 101 &&
           b.last_choice.value == 103);
    assert(b.order_count == 2 && !memcmp(b.order, "LC", 2));
    /* Owner restore is applied by old callback, so aggregate observer is silent. */
    a.mode[3] = b.mode[3] = 1;
    scalar_key(&a, 3, CUI_KEY_DOWN);
    scalar_key(&b, 3, CUI_KEY_DOWN);
    scalar_pair(&a, &b, true);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = scalar_pair(&a, &b, true);
    assert(!r.choice_count && b.choice == 103);
    a.mode[3] = b.mode[3] = 2;
    scalar_key(&a, 3, CUI_KEY_DOWN);
    scalar_key(&b, 3, CUI_KEY_DOWN);
    scalar_pair(&a, &b, true);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = scalar_pair(&a, &b, true);
    assert(r.choice_count == 1 && b.choice == 102);
    assert(b.last_choice.previous == 103 && b.last_choice.value == 102);
    a.mode[3] = b.mode[3] = 0;
    scalar_key(&a, 3, CUI_KEY_DOWN);
    scalar_key(&b, 3, CUI_KEY_DOWN);
    scalar_pair(&a, &b, false);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = scalar_pair(&a, &b, false);
    assert(!r.choice_count && b.choice == 101);
    cui_settings_choice_change copied = b.last_choice;
    scalar_done(&a);
    scalar_done(&b);
    assert(copied.previous == 103 && copied.value == 102);
}

static void text_changes(void) {
    scalar_fixture a, b;
    scalar_init(&a, true, true);
    scalar_init(&b, false, true);
    scalar_fixture *fs[] = {&a, &b};
    for (size_t n = 0; n < 2; ++n) {
        scalar_fixture *f = fs[n];
        f->group.count = 1;
        f->items[0].kind = CUI_SETTING_INPUT;
        f->items[0].edit = f->edit;
        f->items[0].input_state = &f->input;
        assert(cui_edit_replace_range(f->edit, 0, cui_edit_get_state(f->edit).length,
                                      cui_string("")) == CUI_EDIT_OK);
    }
    scalar_pair(&a, &b, true);
    const unsigned modes[] = {0, 1, 1, 2};
    for (size_t k = 0; k < sizeof modes / sizeof modes[0]; ++k) {
        unsigned mode = modes[k];
        a.mode[0] = b.mode[0] = mode;
        for (size_t n = 0; n < 2; ++n) {
            cui_focus(fs[n]->ui, cui_settings_part_id(11, 0, 0, 0, 24));
            assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
            assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "y"}));
        }
        cui_settings_event_result r = scalar_pair(&a, &b, true);
        assert(r.text_count == 1);
        assert(b.last_text.after_bytes == cui_edit_get_state(b.edit).length);
        assert(!strcmp(b.text_copy, mode == 1 ? "" : mode == 2 ? "redirect" : "xy"));
    }
    a.mode[0] = b.mode[0] = 0;
    for (size_t n = 0; n < 2; ++n) {
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START}));
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                              .text_data = "ime", .text_length = 3, .composition_cursor = 3}));
    }
    assert(!scalar_pair(&a, &b, true).text_count);
    for (size_t n = 0; n < 2; ++n)
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                              .text_data = "ime", .text_length = 3, .composition_cursor = 3}));
    assert(scalar_pair(&a, &b, true).text_count == 1);
    for (size_t n = 0; n < 2; ++n) {
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "!"}));
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_TEXT,
                              .text = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"}));
    }
    cui_settings_event_result error = scalar_pair(&a, &b, true);
    assert(error.text_count == 1 && error.result.edit_error != CUI_EDIT_OK);
    for (size_t n = 0; n < 2; ++n)
        assert(cui_queue_event(fs[n]->ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    assert(!scalar_pair(&a, &b, false).text_count);
    a.items[0].disabled = b.items[0].disabled = true;
    assert(!scalar_pair(&a, &b, true).text_count);
    a.items[0].disabled = b.items[0].disabled = false;
    a.passive = b.passive = true;
    assert(!scalar_pair(&a, &b, true).text_count);
    a.passive = b.passive = false;
    a.inert = b.inert = true;
    assert(!scalar_pair(&a, &b, true).text_count);
    cui_settings_text_change copy = b.last_text;
    scalar_done(&a);
    scalar_done(&b);
    assert(copy.after_bytes > 0 && copy.before_revision != copy.after_revision);
}

int main(void) {
    text_changes();
    navigation_events();
    narrow_and_admission();
    scalar_boolean_and_number();
    scalar_choices(false);
    scalar_choices(true);
    return 0;
}
