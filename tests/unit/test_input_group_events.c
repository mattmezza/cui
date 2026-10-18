/* SPDX-License-Identifier: MIT */
#include "activation_assert.h"
#include "cui/cui_form_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    char bytes[32], copies[16][32];
    cui_input_group_state state;
    cui_input_group_options options;
    cui_input_group_addon addons[3];
    bool styled, inert, layout, omit, deferred_omit, redacted;
    size_t count, custom_calls;
    char order[32];
    cui_component_event_metadata events[16];
    cui_component_text_event texts[16];
    cui_draw_list draw;
} fixture;
static void addon(void *user, const cui_component_event_metadata *event) {
    fixture *f = user;
    assert(f->count < 16 && event->kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(event->target == 30 || event->target == 31);
    assert(event->flags == (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
                            CUI_COMPONENT_EVENT_HAS_SOURCE | CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    cui_activation_snapshot actual;
    assert(cui_event_activation_snapshot(f->ui, event->event_index, &actual));
    assert_activation_equal(actual, event->activation);
    size_t count;
    const cui_semantic *semantics = cui_semantics(f->ui, &count);
    assert(count >= 2 && semantics[count - 1].id == event->target);
    assert(semantics[count - 1].details.parent == 10);
    /* Nested Input has not run yet. */
    assert(!cui_edit_get_state(f->edit).length);
    f->order[f->count] = event->target == 30 ? 'A' : 'B';
    f->events[f->count++] = *event;
    f->deferred_omit = true;
}
static void text(void *user, const cui_component_text_event *event) {
    fixture *f = user;
    assert(f->count < 16 && event->event.target == 20);
    assert(!(event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->after.revision == cui_edit_get_state(f->edit).revision);
    assert(f->state.behavior.focus_initialized && f->state.behavior.secure_initialized);
    assert(f->state.behavior.was_masked == f->options.input.masked);
    assert(f->state.behavior.bound_edit_identity == (uintptr_t)f->edit);
    assert(event->has_text == !f->redacted);
    f->texts[f->count] = *event;
    if (event->has_text) {
        cui_str owned;
        assert(cui_component_event_copy_utf8(event->text, 31, f->copies[f->count], 32, &owned) ==
               CUI_COMPONENT_EVENT_COPY_OK);
        f->texts[f->count].text = owned;
    } else
        assert(!event->text.data && !event->text.length);
    f->order[f->count] = event->event.kind == CUI_COMPONENT_EVENT_CHANGE ? 'C' : 'S';
    f->events[f->count++] = event->event;
    f->deferred_omit = true;
}
static void custom(cui_context *ui, const cui_theme *theme, void *user, bool disabled,
                   bool readonly, float width) {
    fixture *f = user;
    (void)theme;
    (void)readonly;
    cui_interaction i = cui_base_button(ui, 60, cui_string("Custom"), disabled);
    cui_begin_box(ui, 60, (cui_box){.width = width, .height = 20});
    cui_end_box(ui);
    if (i.activated)
        ++f->custom_calls;
}
static cui_input_group_event_result frame(fixture *f, bool facade, bool observer) {
    assert(cui_begin_frame(f->ui, 600, 300, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    f->count = 0;
    cui_theme theme = cui_theme_light();
    cui_input_group_event_result out = {0};
    if (!f->omit) {
        if (facade) {
            cui_input_group_handler h = {
                .on_text = observer ? text : NULL, .on_addon = observer ? addon : NULL, .user = f};
            out = f->styled ? cui_input_group_events(f->ui, 10, 20, &theme, f->edit, &f->state,
                                                     f->addons, 3, f->options, h)
                            : cui_base_input_group_events(f->ui, 10, 20, f->edit, &f->state,
                                                          f->addons, 3, f->options, h);
        } else
            out.control = f->styled ? cui_input_group(f->ui, 10, 20, &theme, f->edit, &f->state,
                                                      f->addons, 3, f->options)
                                    : cui_base_input_group(f->ui, 10, 20, f->edit, &f->state,
                                                           f->addons, 3, f->options);
        if (!f->styled) {
            cui_begin_box(f->ui, 10, (cui_box){.width = 400, .height = 50, .row = true});
            for (size_t i = 0; i < 3; ++i) {
                cui_begin_box(f->ui, f->addons[i].id, (cui_box){.width = 40, .height = 40});
                cui_end_box(f->ui);
            }
            cui_begin_box(f->ui, 20, (cui_box){.width = 200, .height = 40});
            cui_end_box(f->ui);
            cui_end_box(f->ui);
        }
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return out;
}
static cui_input_group_event_result pair(fixture *a, fixture *b, bool observer) {
    cui_input_group_event_result x = frame(a, false, false), y = frame(b, true, observer);
    assert(x.control.error == y.control.error &&
           x.control.activated_addon == y.control.activated_addon);
    assert(x.control.input.changed == y.control.input.changed &&
           x.control.input.submitted == y.control.input.submitted &&
           x.control.input.error == y.control.input.error &&
           x.control.input.rejected == y.control.input.rejected);
    assert(!strcmp(a->bytes, b->bytes));
    cui_edit_state as = cui_edit_get_state(a->edit), bs = cui_edit_get_state(b->edit);
    assert(as.revision == bs.revision && as.cursor == bs.cursor && as.anchor == bs.anchor);
    assert(a->state.scroll_x == b->state.scroll_x &&
           !memcmp(&a->state.viewport, &b->state.viewport, sizeof a->state.viewport));
    assert(cui_focused(a->ui) == cui_focused(b->ui) &&
           cui_needs_redraw(a->ui) == cui_needs_redraw(b->ui));
    assert(a->custom_calls == b->custom_calls);
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role && sa[i].value == sb[i].value);
        assert(sa[i].disabled == sb[i].disabled && sa[i].focused == sb[i].focused);
        assert(sa[i].details.parent == sb[i].details.parent &&
               sa[i].details.readonly == sb[i].details.readonly);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(p->text.length == q->text.length && !memcmp(&p->color, &q->color, sizeof p->color));
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
    return y;
}
static void init(fixture *f, bool styled) {
    memset(f, 0, sizeof *f);
    f->styled = styled;
    f->ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->bytes,
                                                .capacity = sizeof f->bytes,
                                                .history_bytes = 2048,
                                                .max_history_steps = 8});
    assert(f->ui && f->edit);
    f->options.width = 400;
    f->options.name = cui_string("Group");
    f->addons[0] = (cui_input_group_addon){
        .id = 30, .kind = CUI_INPUT_GROUP_BUTTON, .label = {"A", 1}, .width = 40};
    f->addons[1] = (cui_input_group_addon){
        .id = 31, .kind = CUI_INPUT_GROUP_BUTTON, .label = {"B", 1}, .width = 40};
    f->addons[2] = (cui_input_group_addon){
        .id = 32, .kind = CUI_INPUT_GROUP_CUSTOM, .render = custom, .user = f, .width = 40};
}
static void send(fixture *a, fixture *b, cui_event e) {
    assert(cui_queue_event(a->ui, e) && cui_queue_event(b->ui, e));
}
static void activate(fixture *a, fixture *b, cui_id id) {
    assert(cui_queue_accessibility_action(a->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(b->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
}
static bool accept(void *user, cui_str candidate) {
    (void)user;
    return !memchr(candidate.data, '!', candidate.length);
}
static void exercise(bool styled, unsigned secure) {
    fixture a, b;
    init(&a, styled);
    init(&b, styled);
    a.options.input.masked = b.options.input.masked = secure == 1;
    a.options.input.content_type = b.options.input.content_type =
        secure == 2 ? CUI_TEXT_INPUT_PURPOSE_PASSWORD : CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
    a.redacted = b.redacted = secure;
    pair(&a, &b, true);
    cui_focus(a.ui, 20);
    cui_focus(b.ui, 20);
    a.state.behavior.focus_initialized = b.state.behavior.focus_initialized = false;
    a.state.behavior.secure_initialized = b.state.behavior.secure_initialized = false;
    activate(&a, &b, 31);
    activate(&a, &b, 30);
    activate(&a, &b, 31);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"});
    send(&a, &b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "y"});
    send(&a, &b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    cui_input_group_event_result r = pair(&a, &b, true);
    assert(r.addon_count == 3 && r.change_count == 2 && r.submit_count == 2);
    assert(r.control.activated_addon == 31 && r.control.input.changed && r.control.input.submitted);
    assert(b.count == 7 && !memcmp(b.order, "ABBCSCS", 7));
    assert(b.events[0].event_index == 1 && b.events[1].event_index == 0 &&
           b.events[2].event_index == 2);
    assert(b.events[3].event_index == 3 && b.events[4].event_index == 4 &&
           b.events[6].event_index == 6);
    if (!secure)
        assert(!strcmp(b.copies[3], "x") && !strcmp(b.copies[5], "xy"));
    a.options.input.validate = b.options.input.validate = accept;
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "!"});
    r = pair(&a, &b, true);
    assert(!r.change_count && r.control.input.rejected);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"});
    send(&a, &b,
         (cui_event){.type = CUI_EVENT_TEXT,
                     .text_data = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
                     .text_length = 52});
    r = pair(&a, &b, true);
    assert(r.change_count == 1 && r.control.input.error != CUI_EDIT_OK);
    if (!secure) {
        send(&a, &b, (cui_event){.type = CUI_EVENT_COMPOSITION_START});
        send(&a, &b,
             (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                         .text_data = "q",
                         .text_length = 1,
                         .composition_cursor = 1});
        assert(!pair(&a, &b, true).change_count);
        send(&a, &b,
             (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                         .text_data = "q",
                         .text_length = 1,
                         .composition_cursor = 1});
        assert(pair(&a, &b, true).change_count == 1);
        send(&a, &b,
             (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL});
        assert(pair(&a, &b, true).change_count == 1);
    }
    a.options.input.readonly = b.options.input.readonly = true;
    /* Addon observer precondition check uses empty text to prove Input has not run. */
    assert(cui_edit_replace_range(a.edit, 0, strlen(a.bytes), cui_string("")) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(b.edit, 0, strlen(b.bytes), cui_string("")) == CUI_EDIT_OK);
    activate(&a, &b, 30);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"});
    r = pair(&a, &b, true);
    assert(r.addon_count == 1 && !r.change_count);
    activate(&a, &b, 30);
    a.addons[0].loading = b.addons[0].loading = true;
    assert(!pair(&a, &b, true).addon_count);
    a.addons[0].loading = b.addons[0].loading = false;
    pair(&a, &b, true);
    activate(&a, &b, 31);
    a.addons[1].disabled = b.addons[1].disabled = true;
    assert(!pair(&a, &b, true).addon_count);
    a.addons[1].disabled = b.addons[1].disabled = false;
    pair(&a, &b, true);
    activate(&a, &b, 30);
    a.options.input.disabled = b.options.input.disabled = true;
    assert(!pair(&a, &b, true).addon_count);
    a.options.input.disabled = b.options.input.disabled = false;
    pair(&a, &b, true);
    a.inert = b.inert = true;
    activate(&a, &b, 30);
    r = pair(&a, &b, true);
    assert(!r.addon_count && !r.change_count && !r.submit_count);
    a.inert = b.inert = false;
    pair(&a, &b, true);
    a.layout = b.layout = true;
    activate(&a, &b, 30);
    r = pair(&a, &b, true);
    assert(!r.addon_count && !r.change_count && !r.submit_count);
    a.layout = b.layout = false;
    pair(&a, &b, true);
    if (styled) {
        activate(&a, &b, 60);
        r = pair(&a, &b, true);
        assert(!r.addon_count && b.custom_calls == 1);
    }
    a.options.input.readonly = b.options.input.readonly = false;
    cui_focus(a.ui, 20);
    cui_focus(b.ui, 20);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "silent"});
    assert(!pair(&a, &b, false).change_count && !strcmp(b.bytes, "silent"));
    cui_component_event_metadata copy = b.events[0];
    a.options.width = b.options.width = -1;
    r = pair(&a, &b, true);
    assert(r.control.error == CUI_FORM_INVALID &&
           r.control.input.error == CUI_EDIT_INVALID_ARGUMENT);
    assert(!r.addon_count && !r.change_count && !r.submit_count && !strcmp(b.bytes, "silent"));
    a.options.width = b.options.width = 400;
    a.omit = b.omit = b.deferred_omit;
    assert(!pair(&a, &b, true).addon_count);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    cui_edit_destroy(a.edit);
    cui_edit_destroy(b.edit);
    assert(copy.target == 30 && copy.kind == CUI_COMPONENT_EVENT_ACTIVATE);
}
static bool drawn(fixture *f, const char *value) {
    size_t n = strlen(value);
    for (size_t i = 0; i < f->draw.count; ++i) {
        cui_str t = f->draw.commands[i].text;
        if (f->draw.commands[i].type == CUI_DRAW_TEXT && t.length == n && !memcmp(t.data, value, n))
            return true;
    }
    return false;
}
static void secret(fixture *f) {
    assert(!drawn(f, "private") && !drawn(f, "leak"));
    size_t n;
    const cui_semantic *s = cui_semantics(f->ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; ++i)
        if (s[i].id == 20) {
            assert(!s[i].value && s[i].details.secret && s[i].details.password);
            found = true;
        }
    assert(found);
}
static cui_edit_result prepared(void *user, cui_str value, bool provisional) {
    (void)value;
    (void)provisional;
    ++*(unsigned *)user;
    return CUI_EDIT_OK;
}
static size_t custom_hit(void *user, cui_str text_value, float x, float y) {
    (void)text_value;
    (void)x;
    (void)y;
    ++*(unsigned *)user;
    return 0;
}
static void secure_paint(void) {
    fixture a, b;
    init(&a, true);
    init(&b, true);
    a.redacted = b.redacted = true;
    a.options.input.masked = b.options.input.masked = true;
    a.options.input.content_type = b.options.input.content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD;
    assert(cui_edit_replace_range(a.edit, 0, 0, cui_string("private")) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(b.edit, 0, 0, cui_string("private")) == CUI_EDIT_OK);
    pair(&a, &b, true);
    assert(drawn(&b, "•••••••"));
    secret(&b);
    cui_focus(a.ui, 20);
    cui_focus(b.ui, 20);
    assert(cui_edit_select(a.edit, 0, 7) == CUI_EDIT_OK);
    assert(cui_edit_select(b.edit, 0, 7) == CUI_EDIT_OK);
    send(&a, &b, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 20});
    assert(!pair(&a, &b, true).change_count);
    send(&a, &b,
         (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                     .composition_target = 20,
                     .text_data = "leak",
                     .text_length = 4,
                     .composition_cursor = 4});
    assert(!pair(&a, &b, true).change_count);
    assert(drawn(&b, "••••"));
    secret(&b);
    send(&a, &b, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL, .composition_target = 20});
    pair(&a, &b, true);
    secret(&b);
    unsigned ha = 0, hb = 0;
    a.options.input.hit_test = b.options.input.hit_test = custom_hit;
    a.options.input.hit_test_user = &ha;
    b.options.input.hit_test_user = &hb;
    cui_rect bounds = cui_bounds(b.ui, 20);
    send(&a, &b, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 2, .y = bounds.y + 2});
    pair(&a, &b, true);
    assert(ha && hb && cui_edit_get_state(b.edit).cursor == 0);
    send(&a, &b, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = bounds.x + 2, .y = bounds.y + 2});
    pair(&a, &b, true);
    a.options.input.hit_test = b.options.input.hit_test = NULL;
    assert(cui_edit_select(a.edit, 0, 7) == CUI_EDIT_OK);
    assert(cui_edit_select(b.edit, 0, 7) == CUI_EDIT_OK);
    /* Actual custom presentation cannot safely coexist with the private mask stage. */
    unsigned ca = 0, cb = 0;
    a.options.input.presentation =
        (cui_edit_presentation){.prepare = prepared, .user = &ca, .user_bytes = sizeof ca};
    b.options.input.presentation =
        (cui_edit_presentation){.prepare = prepared, .user = &cb, .user_bytes = sizeof cb};
    uint64_t revision = cui_edit_get_state(b.edit).revision;
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"});
    cui_input_group_event_result r = pair(&a, &b, true);
    assert(r.control.input.error == CUI_EDIT_INVALID_ARGUMENT && !r.change_count);
    assert(!ca && !cb && cui_edit_get_state(b.edit).revision == revision);
    secret(&b);
    /* Purpose alone intentionally retains visible-password rendering/custom policy. */
    a.options.input.masked = b.options.input.masked = false;
    pair(&a, &b, true);
    cui_focus(a.ui, 20);
    cui_focus(b.ui, 20);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"});
    r = pair(&a, &b, true);
    assert(r.control.input.error == CUI_EDIT_OK && r.change_count == 1 && ca == 1 && cb == 1);
    assert(drawn(&b, "x"));
    secret(&b);
    a.options.input.presentation = b.options.input.presentation = (cui_edit_presentation){0};
    a.options.input.masked = b.options.input.masked = true;
    a.options.input.format.kind = b.options.input.format.kind = CUI_EDIT_FORMAT_NUMBER;
    char wa[4], wb[4];
    a.options.input.format_workspace = (cui_edit_workspace){wa, sizeof wa};
    b.options.input.format_workspace = (cui_edit_workspace){wb, sizeof wb};
    assert(cui_edit_replace_range(a.edit, 0, strlen(a.bytes), cui_string("12")) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(b.edit, 0, strlen(b.bytes), cui_string("12")) == CUI_EDIT_OK);
    pair(&a, &b, true);
    cui_focus(a.ui, 20);
    cui_focus(b.ui, 20);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "3"});
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "4567"});
    r = pair(&a, &b, true);
    assert(r.control.input.error == CUI_EDIT_CAPACITY && r.change_count == 1);
    assert(!strcmp(b.bytes, "123") && drawn(&b, "•••"));
    secret(&b);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    cui_edit_destroy(a.edit);
    cui_edit_destroy(b.edit);
}
static void expired_owned_composition(bool per_event) {
    fixture f;
    init(&f, false);
    f.options.input.masked = true;
    frame(&f, false, false);
    cui_focus(f.ui, 20);
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 20}));
    frame(&f, false, false);
    assert(f.state.behavior.composition_owned && f.state.behavior.composition &&
           f.state.behavior.composition->active);
    /* Two omitted frames cause collection; never inspect the old pointer. */
    f.omit = true;
    frame(&f, false, false);
    frame(&f, false, false);
    f.omit = false;
    f.options.input.masked = false;
    if (per_event) {
        assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "fresh"}));
        assert(cui_begin_frame(f.ui, 600, 300, .016f));
        (void)cui_interact(f.ui, 20, CUI_ROLE_TEXTBOX, cui_string("Field"), false, false, 0);
        cui_focus(f.ui, 20);
        cui_input_result r =
            cui_base_input_reduce_event(f.ui, 20, f.edit, &f.state.behavior, f.options.input, 0);
        assert(r.error == CUI_EDIT_OK);
        cui_end_frame(f.ui);
    } else {
        assert(frame(&f, false, false).control.input.error == CUI_EDIT_OK);
    }
    assert(f.state.behavior.composition_owned && f.state.behavior.composition &&
           !f.state.behavior.composition->active && !f.state.behavior.was_masked);
    cui_destroy(f.ui);
    cui_edit_destroy(f.edit);
}
int main(void) {
    expired_owned_composition(false);
    expired_owned_composition(true);
    secure_paint();
    exercise(false, false);
    exercise(true, false);
    exercise(false, true);
    exercise(true, true);
    exercise(false, 2);
    exercise(true, 2);
    return 0;
}
