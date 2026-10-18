/* SPDX-License-Identifier: MIT */
#include "cui/cui_tree_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_tree_model *model;
    cui_tree_node nodes[5];
    bool expanded[5];
    cui_tree_state state;
    cui_id selected;
    cui_tree_options options;
    cui_tree_selection_change changes[32];
    size_t count;
    cui_tree_expansion_change expansions[32];
    size_t expansion_count;
    cui_tree_activation activations[32];
    size_t activation_count;
    char order[96];
    size_t order_count;
    cui_draw_list draw;
    bool legacy, base, pre_submit, passive, inert, consume;
} fixture;
static void selected(void *user, const cui_tree_selection_change *change) {
    fixture *f = user;
    assert(f->count < 32 && f->selected == change->value && change->previous != change->value);
    assert(change->event.target == 10 && change->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(change->node_index < 5 && f->nodes[change->node_index].key == change->value);
    assert(change->event.flags & CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(change->event.flags & CUI_COMPONENT_EVENT_HAS_INDEX);
    f->changes[f->count++] = *change;
    assert(f->order_count < sizeof f->order);
    f->order[f->order_count++] = 'S';
}
static void expanded(void *user, const cui_tree_expansion_change *change) {
    fixture *f = user;
    assert(f->expansion_count < 32 && change->event.target == 10);
    assert(change->node_index == 0 && change->key == 1);
    assert(change->previous != change->expanded);
    assert(f->expanded[0] == change->expanded);
    assert(cui_tree_expanded(f->model, 0) == change->expanded);
    assert(cui_tree_visible_count(f->model) == (change->expanded ? 5 : 2));
    if (!f->pre_submit) {
        size_t count;
        const cui_semantic *sem = cui_semantics(f->ui, &count);
        for (size_t i = 0; i < count; ++i)
            assert(sem[i].role != CUI_ROLE_TREE);
    }
    f->expansions[f->expansion_count++] = *change;
    assert(f->order_count < sizeof f->order);
    f->order[f->order_count++] = 'E';
}
static void activated(void *user, const cui_tree_activation *event) {
    fixture *f = user;
    assert(f->activation_count < 32 && event->event.target == 10);
    assert(event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(f->selected == event->key && f->state.cursor == event->key);
    assert(f->nodes[event->node_index].key == event->key);
    f->activations[f->activation_count++] = *event;
    assert(f->order_count < sizeof f->order);
    f->order[f->order_count++] = 'A';
}
static cui_tree_event_result frame(fixture *f, bool observe) {
    assert(cui_begin_frame(f->ui, 500, 300, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.layout_only = f->passive, .inert = f->inert});
    if (f->consume)
        assert(cui_consume_event(f->ui, 0));
    if (f->pre_submit)
        cui_tree_submit(f->ui, 10, f->model, &f->state, f->selected, f->options);
    cui_tree_event_result out = {0};
    cui_theme t = cui_theme_light();
    cui_tree_handler handler = {.on_selection_change = observe ? selected : NULL,
                                .on_expansion_change = observe ? expanded : NULL,
                                .on_activate = observe ? activated : NULL,
                                .user = f};
    if (f->pre_submit)
        out = cui_tree_reduce_events(f->ui, 10, f->model, &f->state, &f->selected, f->options,
                                     handler);
    else if (f->base) {
        if (f->legacy)
            out.result = cui_base_tree(f->ui, 10, f->model, &f->state, &f->selected, f->options);
        else
            out = cui_base_tree_events(f->ui, 10, f->model, &f->state, &f->selected, f->options,
                                       handler);
        size_t n;
        const cui_semantic *sem = cui_semantics(f->ui, &n);
        for (size_t i = 0; i < n; ++i) {
            cui_begin_box(f->ui, sem[i].id, (cui_box){.width = 200, .height = 24});
            cui_end_box(f->ui);
        }
    } else if (f->legacy)
        out.result = cui_tree(f->ui, 10, &t, f->model, &f->state, &f->selected, f->options);
    else
        out =
            cui_tree_events(f->ui, 10, &t, f->model, &f->state, &f->selected, f->options, handler);
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK && out.result.error == CUI_TREE_OK);
    return out;
}
static void init(fixture *f, bool legacy, bool base) {
    memset(f, 0, sizeof *f);
    f->legacy = legacy;
    f->base = base;
    f->ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    f->model = cui_tree_create(8);
    assert(f->ui && f->model);
    const cui_tree_node nodes[] = {{1, {"Root", 4}, 0, false},
                                   {2, {"Alpha", 5}, 1, false},
                                   {3, {"Disabled", 8}, 1, true},
                                   {4, {"Beta", 4}, 1, false},
                                   {5, {"Zulu", 4}, 0, false}};
    memcpy(f->nodes, nodes, sizeof nodes);
    f->expanded[0] = true;
    f->selected = 1;
    assert(cui_tree_rebuild(f->model, f->nodes, f->expanded, 5) == CUI_TREE_OK);
    f->options = (cui_tree_options){.width = 240, .height = 200, .row_height = 32, .dt = .016f};
    frame(f, true);
    cui_focus(f->ui, cui_tree_row_id(10, 1));
}
static void done(fixture *f) {
    cui_destroy(f->ui);
    cui_tree_destroy(f->model);
}
static void key(fixture *f, cui_key k) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void click(fixture *f, cui_id key) {
    cui_rect r = cui_bounds(f->ui, cui_tree_row_id(10, key));
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w - 3, .y = r.y + 4}));
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w - 3, .y = r.y + 4}));
}
static void equal(fixture *a, fixture *b, cui_tree_event_result x, cui_tree_event_result y) {
    assert(x.result.selection_changed == y.result.selection_changed &&
           x.result.expansion_changed == y.result.expansion_changed &&
           x.result.activated == y.result.activated && x.result.error == y.result.error);
    assert(a->selected == b->selected && !memcmp(a->expanded, b->expanded, sizeof a->expanded));
    assert(a->state.cursor == b->state.cursor && a->state.scroll == b->state.scroll &&
           a->state.search_length == b->state.search_length &&
           a->state.search_time == b->state.search_time);
    assert(!memcmp(a->state.search, b->state.search, sizeof a->state.search));
    assert(cui_focused(a->ui) == cui_focused(b->ui));
    assert(cui_tree_visible_count(a->model) == cui_tree_visible_count(b->model));
    size_t ne;
    cui_frame_events(a->ui, &ne);
    for (size_t e = 0; e < ne; ++e)
        assert(cui_event_consumed(a->ui, e) == cui_event_consumed(b->ui, e));
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color));
        assert(p->text.length == q->text.length);
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i)
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
               sa[i].details.selected == sb[i].details.selected &&
               sa[i].details.expanded == sb[i].details.expanded);
}
static cui_tree_event_result paired(fixture *a, fixture *b, bool observe) {
    cui_tree_event_result x = frame(a, false), y = frame(b, observe);
    equal(a, b, x, y);
    return y;
}
static void selection(bool base) {
    fixture a, b;
    init(&a, true, base);
    init(&b, false, base);
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    key(&a, CUI_KEY_UP);
    key(&b, CUI_KEY_UP);
    cui_tree_event_result r = paired(&a, &b, true);
    assert(!r.result.selection_changed && r.selection_count == 2 && b.count == 2 &&
           b.selected == 1);
    assert(b.changes[0].previous == 1 && b.changes[0].value == 2);
    assert(b.changes[1].previous == 2 && b.changes[1].value == 1);
    assert(b.changes[0].event.event_index == 0 && b.changes[1].event.event_index == 1);
    assert(b.changes[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(!(b.changes[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    a.options.manual_selection = b.options.manual_selection = true;
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    assert(!paired(&a, &b, true).selection_count);
    click(&a, 4);
    click(&b, 4);
    r = paired(&a, &b, true);
    assert(r.selection_count == 1 && b.selected == 4 && r.result.activated == 4);
    click(&a, 4);
    click(&b, 4);
    assert(!paired(&a, &b, true).selection_count);
    a.options.manual_selection = b.options.manual_selection = false;
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    r = paired(&a, &b, true);
    assert(r.selection_count == 1 && b.selected == 5);
    assert(b.changes[b.count - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    key(&a, CUI_KEY_HOME);
    key(&b, CUI_KEY_HOME);
    r = paired(&a, &b, false);
    assert(r.result.selection_changed && !r.selection_count && b.selected == 1);
    a.consume = b.consume = true;
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    assert(!paired(&a, &b, true).selection_count);
    a.consume = b.consume = false;
    a.options.disabled = b.options.disabled = true;
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    assert(!paired(&a, &b, true).selection_count);
    a.options.disabled = b.options.disabled = false;
    paired(&a, &b, true);
    a.inert = b.inert = true;
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    assert(!paired(&a, &b, true).selection_count);
    a.inert = b.inert = false;
    a.passive = b.passive = true;
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    assert(!paired(&a, &b, true).selection_count);
    a.passive = b.passive = false;
    cui_tree_selection_change copy = b.changes[0];
    assert(cui_begin_frame(b.ui, 500, 300, .016f));
    cui_end_frame(b.ui); /* deferred omission */
    done(&a);
    done(&b);
    assert(copy.value == 2 && copy.event.target == 10);
}
static void accepted_evidence(void) {
    fixture f;
    init(&f, false, false);
    f.pre_submit = true;
    click(&f, 4);
    cui_tree_event_result r = frame(&f, true);
    assert(r.selection_count == 1 && f.count == 1);
    assert(f.changes[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(f.changes[0].event.activation.target == cui_tree_row_id(10, 4));
    assert(f.changes[0].event.target == 10);
    done(&f);
}
static void toggle(fixture *f) {
    cui_rect r = cui_bounds(f->ui, cui_tree_toggle_id(10, 1));
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 2, .y = r.y + 2}));
    assert(cui_queue_event(f->ui,
                           (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 2, .y = r.y + 2}));
}
static void expansion(bool base) {
    fixture a, b;
    init(&a, true, base);
    init(&b, false, base);
    key(&a, CUI_KEY_LEFT);
    key(&b, CUI_KEY_LEFT);
    key(&a, CUI_KEY_RIGHT);
    key(&b, CUI_KEY_RIGHT);
    cui_tree_event_result r = paired(&a, &b, true);
    assert(r.result.expansion_changed && !r.result.selection_changed && r.expansion_count == 2);
    assert(b.expansions[0].previous && !b.expansions[0].expanded);
    assert(!b.expansions[1].previous && b.expansions[1].expanded);
    assert(b.expansions[0].event.event_index == 0 && b.expansions[1].event.event_index == 1);
    assert(b.expansions[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(!(b.expansions[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    toggle(&a);
    toggle(&b);
    toggle(&a);
    toggle(&b);
    r = paired(&a, &b, true);
    assert(r.expansion_count == 2 && b.expanded[0]);
    assert(b.expansions[2].event.event_index == 1 && b.expansions[3].event.event_index == 3);
    assert(b.expansions[2].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    key(&a, CUI_KEY_LEFT);
    key(&b, CUI_KEY_LEFT);
    r = paired(&a, &b, false);
    assert(r.result.expansion_changed && !r.expansion_count && !b.expanded[0]);
    /* Programmatic setter/rebuild is not observed; repair has no expansion event. */
    assert(cui_tree_set_expanded(a.model, 1, true) && cui_tree_set_expanded(b.model, 1, true));
    assert(!paired(&a, &b, true).expansion_count);
    a.options.disabled = b.options.disabled = true;
    key(&a, CUI_KEY_LEFT);
    key(&b, CUI_KEY_LEFT);
    assert(!paired(&a, &b, true).expansion_count);
    a.options.disabled = b.options.disabled = false;
    paired(&a, &b, true);
    a.inert = b.inert = true;
    key(&a, CUI_KEY_LEFT);
    key(&b, CUI_KEY_LEFT);
    assert(!paired(&a, &b, true).expansion_count);
    a.inert = b.inert = false;
    a.passive = b.passive = true;
    key(&a, CUI_KEY_LEFT);
    key(&b, CUI_KEY_LEFT);
    assert(!paired(&a, &b, true).expansion_count);
    cui_tree_expansion_change copy = b.expansions[0];
    done(&a);
    done(&b);
    assert(copy.key == 1 && !copy.expanded);
}
static void expansion_accepted(void) {
    fixture f;
    init(&f, false, false);
    f.pre_submit = true;
    toggle(&f);
    cui_tree_event_result r = frame(&f, true);
    assert(r.expansion_count == 1 && !f.expanded[0]);
    assert(f.expansions[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(f.expansions[0].event.activation.target == cui_tree_toggle_id(10, 1));
    assert(f.expansions[0].event.target == 10);
    done(&f);
}
static void activation(bool base) {
    fixture a, b;
    init(&a, true, base);
    init(&b, false, base);
    click(&a, 4);
    click(&b, 4);
    click(&a, 4);
    click(&b, 4);
    cui_tree_event_result r = paired(&a, &b, true);
    assert(r.selection_count == 1 && r.activation_count == 2 && r.result.activated == 4);
    assert(b.order_count == 3 && !memcmp(b.order, "SAA", 3));
    assert(b.activations[0].event.event_index == 1 && b.activations[1].event.event_index == 3);
    assert(b.activations[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(!(b.activations[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    click(&a, 2);
    click(&b, 2);
    click(&a, 4);
    click(&b, 4);
    r = paired(&a, &b, true);
    assert(r.selection_count == 2 && !r.result.selection_changed && r.activation_count == 2);
    assert(b.order_count == 7 && !memcmp(b.order + 3, "SASA", 4));
    cui_focus(a.ui, cui_tree_row_id(10, 4));
    cui_focus(b.ui, cui_tree_row_id(10, 4));
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    r = paired(&a, &b, true);
    assert(r.activation_count == 2 && !r.selection_count);
    assert(b.activations[4].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    key(&a, CUI_KEY_HOME);
    key(&b, CUI_KEY_HOME);
    r = paired(&a, &b, true);
    assert(!r.activation_count && r.selection_count == 1);
    /* Toggle invokes expansion only, never the row action. */
    toggle(&a);
    toggle(&b);
    r = paired(&a, &b, true);
    assert(r.expansion_count == 1 && !r.activation_count);
    click(&a, 1);
    click(&b, 1);
    r = paired(&a, &b, false);
    assert(r.result.activated == 1 && !r.activation_count);
    a.options.disabled = b.options.disabled = true;
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    assert(!paired(&a, &b, true).activation_count);
    a.options.disabled = b.options.disabled = false;
    paired(&a, &b, true);
    a.inert = b.inert = true;
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    assert(!paired(&a, &b, true).activation_count);
    a.inert = b.inert = false;
    a.passive = b.passive = true;
    key(&a, CUI_KEY_ENTER);
    key(&b, CUI_KEY_ENTER);
    assert(!paired(&a, &b, true).activation_count);
    cui_tree_activation copy = b.activations[0];
    done(&a);
    done(&b);
    assert(copy.key == 4 && copy.event.target == 10);
}
static void activation_accepted(void) {
    fixture f;
    init(&f, false, false);
    f.pre_submit = true;
    click(&f, 4);
    cui_tree_event_result r = frame(&f, true);
    assert(r.selection_count == 1 && r.activation_count == 1);
    assert(f.activations[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(f.activations[0].event.activation.target == cui_tree_row_id(10, 4));
    assert(f.changes[0].event.activation.target == f.activations[0].event.activation.target);
    assert(f.changes[0].event.event_index == f.activations[0].event.event_index);
    done(&f);
}
static void accessibility(void) {
    fixture f;
    init(&f, false, false);
    assert(!cui_accessibility_action_supported(f.ui, cui_tree_row_id(10, 4),
                                               CUI_ACCESSIBILITY_ACTIVATE));
    assert(
        !cui_queue_accessibility_action(f.ui, cui_tree_row_id(10, 4), CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(f.ui) == CUI_ERROR_INVALID_ACTION);
    assert(!f.activation_count && !f.count);
    done(&f);
    init(&f, false, false);
    /* Button toggle is supported; legitimate submit-before-reduce provides evidence. */
    assert(cui_accessibility_action_supported(f.ui, cui_tree_toggle_id(10, 1),
                                              CUI_ACCESSIBILITY_ACTIVATE));
    f.pre_submit = true;
    assert(cui_queue_accessibility_action(f.ui, cui_tree_toggle_id(10, 1),
                                          CUI_ACCESSIBILITY_ACTIVATE));
    cui_tree_event_result r = frame(&f, true);
    assert(r.expansion_count == 1 && !r.activation_count);
    assert(f.expansions[0].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(f.expansions[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(f.expansions[0].event.activation.target == cui_tree_toggle_id(10, 1));
    done(&f);
}
static void stable_keys(void) {
    fixture a, b;
    init(&a, true, false);
    init(&b, false, false);
    cui_tree_node temp = a.nodes[1];
    a.nodes[1] = a.nodes[3];
    a.nodes[3] = temp;
    temp = b.nodes[1];
    b.nodes[1] = b.nodes[3];
    b.nodes[3] = temp;
    assert(cui_tree_rebuild(a.model, a.nodes, a.expanded, 5) == CUI_TREE_OK);
    assert(cui_tree_rebuild(b.model, b.nodes, b.expanded, 5) == CUI_TREE_OK);
    paired(&a, &b, true); /* idle model change emits nothing */
    key(&a, CUI_KEY_DOWN);
    key(&b, CUI_KEY_DOWN);
    cui_tree_event_result r = paired(&a, &b, true);
    assert(r.selection_count == 1 && b.changes[0].value == 4 && b.changes[0].node_index == 1);
    assert(cui_tree_set_expanded(a.model, 1, false) && cui_tree_set_expanded(b.model, 1, false));
    r = paired(&a, &b, true); /* hidden selection stays, cursor repairs without selection change */
    assert(!r.selection_count && !r.expansion_count && !r.activation_count && b.selected == 4);
    assert(b.state.cursor == 1);
    done(&a);
    done(&b);
}
static void admission(void) {
    fixture f;
    init(&f, false, false);
    cui_tree_state before = f.state;
    cui_tree_event_result r =
        cui_tree_reduce_events(f.ui, 0, f.model, &f.state, &f.selected, f.options,
                               (cui_tree_handler){.on_selection_change = selected, .user = &f});
    assert(r.result.error == CUI_TREE_INVALID && !r.selection_count && f.selected == 1);
    assert(!memcmp(&before, &f.state, sizeof before));
    done(&f);
}
int main(void) {
    selection(false);
    selection(true);
    accepted_evidence();
    admission();
    expansion(false);
    expansion(true);
    expansion_accepted();
    activation(false);
    activation(true);
    activation_accepted();
    accessibility();
    stable_keys();
    return 0;
}
