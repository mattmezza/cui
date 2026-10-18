/* SPDX-License-Identifier: MIT */
#include "cui/cui_surface_events.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_surface_event_state *state;
    size_t calls, decisions;
    bool allow;
    cui_surface_lifecycle_event saved;
} observer;
static void closed(void *user, const cui_surface_lifecycle_event *event) {
    observer *o = user;
    assert(!o->state->control.overlay.open);
    assert(cui_focused(o->ui) == 1);
    assert(event->event.target == 100 && event->event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    assert(event->event.flags == 0 && event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(!cui_surface_events_detach(o->state));
    assert(!cui_surface_events_set_handler(o->state, (cui_surface_event_handler){0}));
    assert(!cui_base_surface_update_events(o->ui, o->state, (cui_surface_options){0}).valid);
    o->saved = *event;
    o->calls++;
}
static bool decide(void *user, cui_surface_result result) {
    observer *o = user;
    assert(result != CUI_SURFACE_NONE);
    assert(o->state->control.overlay.open);
    o->decisions++;
    return o->allow;
}
static void legacy_closed(void *user, cui_surface_result result) {
    (void)user; (void)result;
}
static cui_surface_event_result scene(observer *o, cui_surface_options options, unsigned mode) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(o->ui, 600, 400, .016f));
    cui_begin_box(o->ui, 900, (cui_box){.width = 600, .height = 400});
    cui_button(o->ui, 1, &theme, cui_string("Open"), false);
    cui_surface_event_result result;
    if (mode == 0)
        result = cui_base_surface_update_events(o->ui, o->state, options);
    else if (mode == 1) {
        result = cui_base_surface_begin_events(o->ui, o->state, cui_string("Title"), cui_string("Body"), options);
        if (result.control_result) cui_base_surface_end(o->ui);
    } else if (mode == 2) {
        result = cui_surface_begin_events(o->ui, &theme, o->state, cui_string("Title"), cui_string("Body"), options);
        if (result.control_result) cui_surface_end(o->ui);
    } else
        result = cui_alert_dialog_events(o->ui, &theme, o->state, cui_string("Title"), cui_string("Body"), cui_string("OK"), cui_string("Cancel"), options);
    cui_end_box(o->ui);
    cui_end_frame(o->ui);
    assert(cui_last_error(o->ui) == CUI_OK);
    return result;
}
static void direct_close(observer *o, cui_surface_result result) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(o->ui, 600, 400, .016f));
    cui_begin_box(o->ui, 900, (cui_box){.width = 600, .height = 400});
    cui_button(o->ui, 1, &theme, cui_string("Open"), false);
    cui_surface_close(o->ui, &o->state->control, result);
    cui_end_box(o->ui);
    cui_end_frame(o->ui);
    assert(cui_last_error(o->ui) == CUI_OK);
}
static void lifecycle(void) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
        assert(ui);
        cui_surface_event_state state;
        cui_surface_events_init(&state, 100, mode == 3 ? CUI_SURFACE_ALERT : CUI_SURFACE_DIALOG);
        observer o = {.ui = ui, .state = &state};
        assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &o}));
        cui_surface_options options = {.viewport = {0, 0, 600, 400}, .width = 300, .height = 220,
                                       .decide = decide, .decision_user = &o};
        assert(scene(&o, options, mode).valid);
        cui_surface_open(ui, &state.control, 1);
        assert(!cui_surface_events_detach(&state));
        assert(scene(&o, options, mode).close_count == 0);
        options.width = 320; /* Refresh must retain adapter and decision owner. */
        cui_surface_request_close(ui, &state.control, CUI_SURFACE_ACCEPTED);
        assert(scene(&o, options, mode).close_count == 0 && o.decisions == 1);
        assert(state.control.overlay.open && state.control.pending_close == CUI_SURFACE_NONE);
        assert(scene(&o, options, mode).close_count == 0 && o.decisions == 1);
        o.allow = true;
        cui_surface_request_close(ui, &state.control, CUI_SURFACE_ACCEPTED);
        cui_surface_request_close(ui, &state.control, CUI_SURFACE_CANCELLED);
        cui_surface_event_result result = scene(&o, options, mode);
        assert(result.valid && result.close_count == 1 && o.calls == 1 && o.decisions == 2);
        assert(o.saved.result == CUI_SURFACE_CANCELLED);
        assert(cui_surface_take_result(&state.control) == CUI_SURFACE_CANCELLED);
        assert(cui_surface_take_result(&state.control) == CUI_SURFACE_NONE);
        assert(scene(&o, options, mode).close_count == 0 && o.calls == 1);
        cui_surface_open(ui, &state.control, 1);
        direct_close(&o, CUI_SURFACE_ACCEPTED); /* bypasses decide */
        assert(o.calls == 2 && o.decisions == 2 && o.saved.result == CUI_SURFACE_ACCEPTED);
        direct_close(&o, CUI_SURFACE_CLOSED);
        assert(o.calls == 2);
        assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){0}));
        cui_surface_open(ui, &state.control, 1);
        cui_surface_request_close(ui, &state.control, CUI_SURFACE_CLOSED);
        assert(scene(&o, options, mode).close_count == 0 && o.calls == 2);
        assert(!state.control.overlay.open);
        assert(cui_surface_events_detach(&state));
        assert(!state.control.on_close && !state.control.close_user);
        assert(cui_surface_take_result(&state.control) == CUI_SURFACE_CLOSED);
        assert(!cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &o}));
        cui_surface_events_init(&state, 100, CUI_SURFACE_DIALOG);
        assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &o}));
        cui_surface_open(ui, &state.control, 1);
        direct_close(&o, CUI_SURFACE_CLOSED);
        assert(o.calls == 3 && o.saved.result == CUI_SURFACE_CLOSED);
        assert(cui_surface_events_detach(&state));
        cui_destroy(ui);
    }
}
static void conflicts_and_helper(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_surface_event_state state;
    cui_surface_events_init(&state, 100, CUI_SURFACE_DIALOG);
    observer o = {.ui = ui, .state = &state};
    cui_surface_options options = {.viewport = {0, 0, 600, 400}, .width = 300, .height = 200};
    scene(&o, options, 2);
    cui_surface_open(ui, &state.control, 1);
    cui_surface_request_close(ui, &state.control, CUI_SURFACE_ACCEPTED);
    for (unsigned i = 0; i < 2; ++i) {
        cui_surface_options conflict = options;
        if (i) conflict.close_user = &o; else conflict.on_close = legacy_closed;
        cui_surface_event_state before = state;
        cui_theme theme = cui_theme_light();
        assert(!cui_base_surface_update_events(ui, &state, conflict).valid);
        assert(!cui_base_surface_begin_events(ui, &state, cui_string("T"), cui_string("D"), conflict).valid);
        assert(!cui_surface_begin_events(ui, &theme, &state, cui_string("T"), cui_string("D"), conflict).valid);
        assert(!cui_alert_dialog_events(ui, &theme, &state, cui_string("T"), cui_string("D"), cui_string("Y"), cui_string("N"), conflict).valid);
        assert(memcmp(&before, &state, sizeof state) == 0);
        cui_surface_options output; memset(&output, 0x5a, sizeof output);
        cui_surface_options previous = output;
        assert(!cui_surface_events_options(&state, conflict, &output));
        assert(memcmp(&output, &previous, sizeof output) == 0);
    }
    assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &o}));
    cui_surface_options transformed;
    assert(cui_surface_events_options(&state, options, &transformed));
    assert(transformed.width == options.width && transformed.on_close && transformed.close_user == &state);
    assert(cui_begin_frame(ui, 600, 400, .016f));
    assert(cui_base_surface_update(ui, &state.control, transformed));
    cui_end_frame(ui);
    assert(o.calls == 1); /* helper preserves retention without wrapper count */
    cui_surface_open(ui, &state.control, 1);
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_base_surface_update(ui, &state.control, options); /* explicitly unsupported mixing */
    cui_end_frame(ui);
    assert(!cui_base_surface_update_events(ui, &state, options).valid);
    assert(!cui_surface_events_detach(&state));
    direct_close(&o, CUI_SURFACE_CLOSED);
    assert(o.calls == 1);
    cui_destroy(ui);
}
static void retained_after_begin(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_surface_event_state state;
    cui_surface_events_init(&state, 100, CUI_SURFACE_DIALOG);
    observer o = {.ui = ui, .state = &state};
    assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &o}));
    cui_surface_options options = {.viewport = {0, 0, 600, 400}, .width = 300, .height = 200};
    scene(&o, options, 2);
    cui_surface_open(ui, &state.control, 1);
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 400});
    cui_button(ui, 1, &theme, cui_string("Open"), false);
    cui_surface_event_result result = cui_surface_begin_events(ui, &theme, &state,
        cui_string("Title"), cui_string("Body"), options);
    assert(result.valid && result.control_result && result.close_count == 0 && !state.active);
    cui_surface_close(ui, &state.control, CUI_SURFACE_CLOSED);
    assert(o.calls == 1 && result.close_count == 0); /* Returned values never mutate. */
    cui_surface_end(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_surface_event_state copied = state;
    cui_surface_options output;
    assert(!cui_surface_events_options(&copied, options, &output));
    assert(!cui_surface_events_detach(&copied));
    cui_surface_event_state before = state;
    assert(!cui_surface_events_set_handler(&state, (cui_surface_event_handler){NULL, &o}));
    assert(memcmp(&before, &state, sizeof state) == 0);
    assert(cui_surface_events_detach(&state));
    assert(o.saved.result == CUI_SURFACE_CLOSED && o.saved.event.target == 100);
    cui_destroy(ui);
}
static void sheet_replacement(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_surface_event_state state;
    cui_surface_events_init(&state, 100, CUI_SURFACE_SHEET);
    observer first = {.ui = ui, .state = &state}, second = first;
    assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &first}));
    cui_surface_options options = {.viewport = {0, 0, 600, 400}, .width = 200, .height = 200,
                                   .sheet_animation_seconds = .064f};
    scene(&first, options, 2);
    cui_surface_open(ui, &state.control, 1);
    for (unsigned i = 0; i < 6; ++i) scene(&first, options, 2);
    cui_surface_request_close(ui, &state.control, CUI_SURFACE_CANCELLED);
    assert(scene(&first, options, 2).close_count == 0 && state.control.sheet_closing);
    assert(!cui_surface_events_detach(&state));
    assert(cui_surface_events_set_handler(&state, (cui_surface_event_handler){closed, &second}));
    size_t count = 0;
    for (unsigned i = 0; i < 8; ++i) count += scene(&second, options, 2).close_count;
    assert(count == 1 && first.calls == 0 && second.calls == 1);
    assert(second.saved.result == CUI_SURFACE_CANCELLED);
    assert(cui_surface_events_detach(&state));
    cui_destroy(ui);
}
int main(void) { lifecycle(); conflicts_and_helper(); retained_after_begin(); sheet_replacement(); return 0; }
