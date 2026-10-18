/* SPDX-License-Identifier: MIT */
#include "cui/cui_settings_events.h"
#include <assert.h>
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
        out = cui_settings_events(f->ui, 11, &theme, f->pages, 2, &f->state, f->options,
                                  (cui_settings_handler){observe ? navigation : NULL, f});
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
                            (cui_settings_handler){navigation, &b});
    assert(r.result.invalid && !r.navigation_count && !memcmp(&before, &b.state, sizeof before));
    done(&a);
    done(&b);
}
int main(void) {
    navigation_events();
    narrow_and_admission();
    return 0;
}
