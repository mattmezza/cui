/* SPDX-License-Identifier: MIT */
#include "cui/cui_button_group.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    bool selected[3], accept;
    size_t calls, order[16], counts[16], requests[16][3];
    cui_button_group_item *source;
} model;
static void changed(void *user, const cui_button_group_request *request) {
    model *m = user;
    size_t n = m->calls++;
    assert(n < 16);
    m->order[n] = request->item_index;
    m->counts[n] = request->selected_count;
    memcpy(m->requests[n], request->selected_indices, request->selected_count * sizeof(size_t));
    if (m->accept) {
        memset(m->selected, 0, sizeof m->selected);
        for (size_t i = 0; i < request->selected_count; i++)
            m->selected[request->selected_indices[i]] = true;
    }
    if (m->source)
        m->source[0].disabled = true; /* Descriptors were snapshotted. */
}
static unsigned child_calls;
static void child(void *user) {
    ++*(unsigned *)user;
}
static cui_button_group_result scene(cui_context *ui, cui_button_group_workspace *w, model *m,
                                     cui_button_group_item *items, cui_button_group_options o) {
    assert(cui_begin_frame(ui, 320, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 320, .height = 100});
    cui_begin_absolute_box(ui, 100, (cui_box){0}, (cui_rect){10, 10, 280, 60});
    assert(cui_base_button_group_begin(ui, 100, items, m->selected, 3, w, o));
    for (size_t i = 0; i < 3; i++) {
        cui_begin_absolute_box(ui, items[i].id, (cui_box){0}, (cui_rect){(float)i * 80, 0, 80, 32});
        cui_base_button_group_item(ui, w, i);
        cui_end_box(ui);
    }
    cui_button_group_result result = cui_base_button_group_end(ui, w);
    assert(result.valid);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    return result;
}
static void click(cui_context *ui, size_t index, bool touch) {
    float x = 30 + (float)index * 80;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                        .x = x,
                        .y = 20,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .pointer_id = 7}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                        .x = x,
                        .y = 20,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .pointer_id = 7}));
}
static void assert_checked(cui_context *ui, model *m) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (s[i].id >= 10 && s[i].id <= 12)
            assert(s[i].checked == m->selected[s[i].id - 10] &&
                   s[i].value == (m->selected[s[i].id - 10] ? 1 : 0));
}
int main(void) {
    cui_button_group_workspace *w = calloc(1, sizeof *w);
    assert(w);
    for (unsigned multiple = 0; multiple < 2; multiple++)
        for (unsigned accept = 0; accept < 2; accept++) {
            model m = {.selected = {false, true, false}, .accept = accept};
            cui_context *ui = cui_create((cui_config){0});
            assert(ui);
            cui_button_group_item items[3] = {{.id = 10, .label = {"One", 3}},
                                              {.id = 11, .label = {"Two", 3}},
                                              {.id = 12, .label = {"Three", 5}}};
            cui_button_group_options o = {
                .multiple = multiple, .on_change = changed, .change_user = &m};
            scene(ui, w, &m, items, o);
            scene(ui, w, &m, items, o);
            /* Queue in the opposite order to child traversal, and twice on one child. */
            click(ui, 2, false);
            click(ui, 0, false);
            click(ui, 0, true);
            m.source = items;
            cui_button_group_result r = scene(ui, w, &m, items, o);
            assert(r.accepted_activations == 3 && r.callbacks == 3 && m.calls == 3);
            assert(m.order[0] == 2 && m.order[1] == 0 && m.order[2] == 0);
            if (multiple) {
                assert(m.counts[0] == 2 && m.requests[0][0] == 1 && m.requests[0][1] == 2);
                if (accept)
                    assert(m.counts[1] == 3 && m.counts[2] == 2 && m.requests[2][1] == 2);
                else
                    assert(m.counts[1] == 2 && m.requests[1][0] == 1 && m.requests[1][1] == 0);
            } else
                assert(m.counts[0] == 1 && m.requests[0][0] == 2 && m.requests[1][0] == 0);
            assert_checked(ui, &m);
            m.source = NULL;
            items[0].disabled = false;
            m.calls = 0;
            scene(ui, w, &m, items, o);
            cui_focus(ui, 10);
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            r = scene(ui, w, &m, items, o);
            assert(r.accepted_activations == 1 && r.callbacks == 0 && m.calls == 0);
            o.activation_policy = CUI_BUTTON_GROUP_ALL_ACTIVATIONS;
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
            r = scene(ui, w, &m, items, o);
            assert(r.callbacks == 1 && m.calls == 1);
            m.calls = 0;
            items[0].loading = true;
            click(ui, 0, true);
            r = scene(ui, w, &m, items, o);
            assert(!r.callbacks && !m.calls);
            items[0].loading = false;
            /* Cancel never dispatches; a later ordinary tap remains usable. */
            m.calls = 0;
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 30, .y = 20}));
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 30, .y = 20}));
            r = scene(ui, w, &m, items, o);
            assert(!r.callbacks && !m.calls);
            click(ui, 0, false);
            r = scene(ui, w, &m, items, o);
            assert(r.callbacks == 1);
            /* Background identity has no sticky child index and leaves selection unchanged. */
            m.calls = 0;
            bool before[3];
            memcpy(before, m.selected, sizeof before);
            assert(cui_queue_event(ui,
                                   (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 270, .y = 60}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 270, .y = 60}));
            r = scene(ui, w, &m, items, o);
            assert(r.callbacks == 1 && m.order[0] == SIZE_MAX);
            assert(!memcmp(before, m.selected, sizeof before));
            m.calls = 0;
            o.disabled = true;
            scene(ui, w, &m, items, o);
            click(ui, 0, true);
            r = scene(ui, w, &m, items, o);
            assert(!r.callbacks && !m.calls);
            o.disabled = false;
            scene(ui, w, &m, items, o);
            o.on_change = NULL;
            for (size_t i = 0; i < 3; i++) {
                items[i].on_click = child;
                items[i].action_user = &child_calls;
            }
            scene(ui, w, &m, items, o);
            child_calls = 0;
            click(ui, 1, false);
            click(ui, 1, true);
            r = scene(ui, w, &m, items, o);
            assert(r.callbacks == 2 && child_calls == 2);
            /* Reordered descriptors keep stable IDs; indices describe the new order. */
            cui_button_group_item tmp = items[0];
            items[0] = items[2];
            items[2] = tmp;
            scene(ui, w, &m, items, o);
            scene(ui, w, &m, items, o);
            click(ui, 0, true);
            r = scene(ui, w, &m, items, o);
            assert(r.callbacks == 1);
            cui_destroy(ui);
        }
    cui_context *a = cui_create((cui_config){0}), *b = cui_create((cui_config){0});
    assert(a && b);
    cui_button_group_item one = {.id = 10, .label = {"One", 3}};
    bool value = false;
    assert(cui_begin_frame(a, 100, 100, 0));
    assert(cui_base_button_group_begin(a, 100, &one, &value, 1, w, (cui_button_group_options){0}));
    assert(!cui_base_button_group_begin(a, 101, &one, &value, 1, w, (cui_button_group_options){0}));
    assert(!cui_base_button_group_end(a, w).valid); /* Missing item, workspace released. */
    assert(cui_base_button_group_begin(a, 102, &one, &value, 1, w, (cui_button_group_options){0}));
    cui_end_frame(a);
    assert(cui_begin_frame(a, 100, 100, 0));
    assert(!cui_base_button_group_end(a, w).valid); /* Stale even with dt=0. */
    assert(cui_base_button_group_begin(a, 103, &one, &value, 1, w, (cui_button_group_options){0}));
    assert(cui_begin_frame(b, 100, 100, 0));
    assert(!cui_base_button_group_end(b, w).valid); /* Cross context rejected. */
    cui_end_frame(b);
    cui_end_frame(a);
    /* Empty, single and maximum bounded collection; no model mutation without a handler. */
    cui_button_group_item *many = calloc(CUI_BUTTON_GROUP_MAX_ITEMS, sizeof *many);
    bool *many_values = calloc(CUI_BUTTON_GROUP_MAX_ITEMS, sizeof *many_values);
    assert(many && many_values);
    for (size_t i = 0; i < CUI_BUTTON_GROUP_MAX_ITEMS; i++)
        many[i] = (cui_button_group_item){.id = 1000 + i};
    for (unsigned n = 0; n < 3; n++) {
        size_t count = n == 0 ? 0 : n == 1 ? 1 : CUI_BUTTON_GROUP_MAX_ITEMS;
        assert(cui_begin_frame(a, 100, 100, 0));
        assert(cui_base_button_group_begin(a, 100, many, many_values, count, w,
                                           (cui_button_group_options){0}));
        for (size_t i = 0; i < count; i++)
            cui_base_button_group_item(a, w, i);
        assert(cui_base_button_group_end(a, w).valid);
        cui_end_frame(a);
    }
    assert(cui_begin_frame(a, 100, 100, 0));
    many[1].id = many[0].id;
    assert(!cui_base_button_group_begin(a, 100, many, many_values, 2, w,
                                        (cui_button_group_options){0}));
    assert(!cui_base_button_group_begin(a, 100, many, many_values, CUI_BUTTON_GROUP_MAX_ITEMS + 1,
                                        w, (cui_button_group_options){0}));
    assert(cui_base_button_group_begin(a, 100, &one, &value, 1, w, (cui_button_group_options){0}));
    cui_base_button_group_item(a, w, 0);
    cui_base_button_group_item(a, w, 0);
    assert(!cui_base_button_group_end(a, w).valid);
    cui_end_frame(a);
    /* Core capacity rejection releases the staged workspace without callbacks. */
    cui_context *small = cui_create((cui_config){.max_elements = 1});
    assert(small);
    assert(cui_begin_frame(small, 100, 100, 0));
    assert(
        cui_base_button_group_begin(small, 100, &one, &value, 1, w, (cui_button_group_options){0}));
    cui_base_button_group_item(small, w, 0);
    assert(cui_last_error(small) == CUI_ERROR_CAPACITY);
    assert(!cui_base_button_group_end(small, w).valid && !w->active);
    cui_end_frame(small);
    cui_destroy(small);
    free(many);
    free(many_values);
    cui_destroy(a);
    cui_destroy(b);
    free(w);
}
