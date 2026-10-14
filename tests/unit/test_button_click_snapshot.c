/* SPDX-License-Identifier: MIT */
#define main legacy_click_tests
#include "test_button_click_event.c"
#undef main
static cui_button_click_event evidence_log[32];
static unsigned evidence_count, evidence_legacy;
static cui_context *evidence_ui;
static bool consume_access;
static float scene_padding;
static void evidence_old(void *user) {
    (void)user;
    ++evidence_legacy;
    if (consume_access) {
        size_t count;
        const cui_event *events = cui_frame_events(evidence_ui, &count);
        for (size_t i = 0; i < count; i++)
            if (events[i].type == CUI_EVENT_ACCESSIBILITY_ACTION)
                assert(cui_consume_event(evidence_ui, i));
    }
}
static void evidence_new(void *user, const cui_button_click_event *event) {
    (void)user;
    assert(evidence_legacy == evidence_count + 1);
    assert(event->has_evidence && event->evidence.target == event->target);
    assert(event->evidence.event_index == event->event_index);
    assert(event->evidence.frame_sequence == event->frame_sequence);
    assert(event->evidence.event_type == event->event_type);
    evidence_log[evidence_count++] = *event;
}
static void evidence_frame(cui_context *ui, cui_button_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 1000, 500, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 800, .height = 400, .padding = scene_padding});
    (void)cui_button_ex(ui, 10, &theme, cui_string("Evidence"), options);
    cui_end_box(ui);
    cui_end_frame(ui);
}
static cui_event sample(cui_context *ui, cui_event_type type, cui_pointer_type kind,
                        uint64_t contact, float offset, unsigned mods, unsigned clicks) {
    cui_rect r = cui_bounds(ui, 10);
    return (cui_event){.type = type,
                       .x = r.x + offset,
                       .y = r.y + 5,
                       .pointer_type = kind,
                       .pointer_id = contact,
                       .pointer_device = 19,
                       .modifiers = mods,
                       .click_count = clicks};
}
static unsigned preparation, compatibility;
static cui_activation_snapshot prepared;
static void prepare_child(void *user, cui_context *ui, const cui_activation_snapshot *snapshot) {
    (void)user;
    assert(ui == evidence_ui && preparation == compatibility);
    prepared = *snapshot;
    preparation++;
}
static void compatibility_child(void *user, cui_context *ui, size_t index) {
    (void)user;
    assert(ui == evidence_ui && preparation == compatibility + 1);
    assert(prepared.event_index == index);
    assert(evidence_legacy == compatibility + 1);
    compatibility++;
}
static void base_group_frame(cui_context *ui, cui_button_group_workspace *workspace,
                             bool override) {
    cui_button_group_item item = {.id = 10,
                                  .label = {"Evidence", 8},
                                  .on_click = evidence_old,
                                  .prepare_click_event = prepare_child,
                                  .on_click_event = compatibility_child};
    bool selected = false;
    assert(cui_begin_frame(ui, 1000, 500, 0));
    assert(cui_base_button_group_begin(
        ui, 100, &item, &selected, 1, workspace,
        (cui_button_group_options){.on_change = override ? changed : NULL,
                                   .activation_policy = CUI_BUTTON_GROUP_ALL_ACTIVATIONS}));
    cui_begin_box(ui, 100, (cui_box){.width = 800, .height = 400});
    (void)cui_base_button_group_item(ui, workspace, 0);
    cui_begin_box(ui, 10, (cui_box){.width = 120, .height = 40});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_button_group_result result = cui_base_button_group_end(ui, workspace);
    assert(result.valid);
    if (result.accepted_activations)
        assert(result.callbacks == (override ? 1 : 2));
    cui_end_frame(ui);
}
#ifndef CUI_BUTTON_SNAPSHOT_MAIN
#define CUI_BUTTON_SNAPSHOT_MAIN main
#endif
int CUI_BUTTON_SNAPSHOT_MAIN(void) {
    legacy_click_tests(); /* Existing callback/multiplicity/passive contracts stay intact. */
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 65536,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    evidence_ui = ui;
    cui_button_options options = {.on_click = evidence_old, .on_click_event = evidence_new};
    evidence_frame(ui, options);
    for (unsigned click = 0; click < 3; click++) {
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 0,
                                          4 + click, CUI_MOD_SHIFT, 0)));
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 0, 5 + click,
                                          CUI_MOD_CTRL, click)));
    }
    evidence_frame(ui, options);
    assert(evidence_count == 3);
    for (unsigned click = 0; click < 3; click++) {
        cui_activation_snapshot s = evidence_log[click].evidence;
        assert(s.event_index == 2 * click + 1 && s.has_press && s.has_release);
        assert(s.press.x == 4 + click && s.release.x == 5 + click);
        assert(s.press.modifiers == CUI_MOD_SHIFT && s.release.modifiers == CUI_MOD_CTRL);
        assert(s.press.click_count == 0 && s.release.click_count == click);
        assert(evidence_log[click].click_count == (click ? click : 1));
    }
    for (unsigned kind = 0; kind < 3; kind++) {
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_DOWN, (cui_pointer_type)kind,
                                          kind ? 71 : 0, 4, CUI_MOD_SHIFT, 1)));
        evidence_frame(ui, options);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_LEAVE}));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_ENTER}));
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_UP, (cui_pointer_type)kind,
                                          kind ? 71 : 0, 6, CUI_MOD_ALT, 2)));
        evidence_frame(ui, options);
        const cui_activation_snapshot *s = &evidence_log[evidence_count - 1].evidence;
        assert(s->has_press && s->has_release && s->event_index == 2);
        assert(s->press.x == 4 && s->release.x == 6 && s->press.type == (cui_pointer_type)kind);
        assert(s->press.device == 19 && s->release.device == 19);
    }
    assert(evidence_count == 6);
    assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 7, 4, 0, 1)));
    assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 8, 6, 0, 1)));
    evidence_frame(ui, options);
    assert(evidence_count == 7 && !evidence_log[6].evidence.has_press &&
           evidence_log[6].evidence.has_release);
    assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 0, 4, 0, 1)));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 0, 6, 0, 1)));
    evidence_frame(ui, options);
    assert(evidence_count == 7);
    cui_rect previous = cui_bounds(ui, 10);
    cui_focus(ui, 10);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_CTRL}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    scene_padding = 40;
    evidence_frame(ui, options);
    assert(evidence_count == 8 && evidence_log[7].evidence.has_key_bounds);
    assert(!memcmp(&previous, &evidence_log[7].evidence.key_bounds, sizeof(previous)));
    assert(cui_bounds(ui, 10).x != previous.x);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    consume_access = true;
    evidence_frame(ui, options);
    assert(evidence_count == 9 && !evidence_log[8].evidence.has_press &&
           !evidence_log[8].evidence.has_release && !evidence_log[8].evidence.has_key_bounds);
    consume_access = false;
    cui_button_group_style_workspace *workspace = calloc(1, sizeof(*workspace));
    assert(workspace);
    cui_button_group_styled_item item = {.id = 10, .label = {"Evidence", 8}, .options = options};
    bool selected = false;
    cui_theme theme = cui_theme_light();
    for (unsigned pass = 0; pass < 2; pass++) {
        if (pass)
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                .accessibility_target = 10,
                                .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        assert(cui_begin_frame(ui, 1000, 500, 0));
        cui_button_group_result result;
        consume_access = pass != 0;
        assert(cui_button_group_ex(ui, 100, &theme, &item, &selected, 1, workspace,
                                   (cui_button_group_style_options){0}, &result));
        if (pass)
            assert(result.callbacks == 2);
        cui_end_frame(ui);
    }
    assert(evidence_count == 10 && evidence_log[9].has_evidence);
    consume_access = false;
    for (unsigned click = 0; click < 3; click++) {
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 0,
                                          4 + click, CUI_MOD_SHIFT, click)));
        assert(cui_queue_event(ui, sample(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 0, 5 + click,
                                          CUI_MOD_CTRL, click)));
    }
    assert(cui_begin_frame(ui, 1000, 500, 0));
    cui_button_group_result repeated;
    assert(cui_button_group_ex(ui, 100, &theme, &item, &selected, 1, workspace,
                               (cui_button_group_style_options){0}, &repeated));
    cui_end_frame(ui);
    assert(repeated.callbacks == 6 && evidence_count == 13);
    for (unsigned click = 0; click < 3; click++) {
        const cui_activation_snapshot *snapshot = &evidence_log[10 + click].evidence;
        assert(snapshot->has_press && snapshot->has_release &&
               snapshot->event_index == 2 * click + 1);
        assert(snapshot->press.x == 4 + click && snapshot->release.x == 5 + click);
    }
    free(workspace);
    cui_button_click_event retained = evidence_log[0];
    cui_destroy(ui);
    assert(retained.evidence.press.x == 4 && retained.evidence.release.x == 5);
    ui = cui_create((cui_config){.max_elements = 128, .max_commands = 512});
    assert(ui);
    evidence_ui = ui;
    evidence_legacy = 0;
    cui_button_group_workspace base = {0};
    base_group_frame(ui, &base, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    consume_access = true;
    base_group_frame(ui, &base, false);
    assert(preparation == 1 && compatibility == 1 && evidence_legacy == 1);
    consume_access = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    base_group_frame(ui, &base, true);
    assert(preparation == 1 && compatibility == 1 && evidence_legacy == 1);
    cui_destroy(ui);
    return 0;
}
