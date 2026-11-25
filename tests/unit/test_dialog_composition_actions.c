/* SPDX-License-Identifier: MIT */
#include "dialog_composition_helpers.h"
static void wheel(dialog_fixture *f, float dy) {
    cui_rect b = f->layout.completed[5];
    assert(b.w > 0 && b.h > 0);
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 20, .y = b.y + 20}));
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = b.x + 20, .y = b.y + 20, .dy = dy}));
}
static void actions_and_reopen(bool styled) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    f.action_footer = true;
    f.metric[CUI_DIALOG_BODY] = 800;
    f.composition.present = (1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_FOOTER);
    dialog_fixture_frame(&f);
    const cui_semantic *action = dialog_semantic(&f, 101);
    assert(action && !action->disabled); /* first publication is enabled */
    float published_maximum = dialog_semantic(&f, cui_surface_part_id(50, 6))->details.scroll_max_y;
    assert(published_maximum > 0);
    wheel(&f, -2);
    dialog_fixture_frame(&f);
    dialog_equal(f.surface.body_maximum, published_maximum, "admitted prior input maximum");
    assert(f.surface.body_scroll > 0);
    f.veto = true;
    dialog_fixture_pointer(&f, 101);
    dialog_fixture_frame(&f);
    assert(f.decisions == 1 && f.closes == 0 && f.surface.overlay.open);
    assert(f.decision == CUI_SURFACE_ACCEPTED);
    f.veto = false;
    dialog_fixture_pointer(&f, 101);
    dialog_fixture_frame(&f);
    assert(f.decisions == 2 && f.closes == 1 && !f.surface.overlay.open);
    assert(f.closed_reason == CUI_SURFACE_ACCEPTED);
    assert(f.focus_closed == 1);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    assert(f.surface.body_scroll == 0);
    dialog_fixture_frame(&f);
    assert(f.surface.overlay.open);
    /* Close/open between frames, with pointer+wheel routed against the old
     * snapshot. Neither may become an action/scroll in the new lifetime. */
    dialog_fixture_pointer(&f, 101);
    wheel(&f, -2);
    /* Explicit controlled-state closure uses the existing between-frame-safe
     * overlay helper; surface_request_close retains deferred policy timing. */
    cui_overlay_close(f.ui, &f.surface.overlay);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    f.metric[CUI_DIALOG_BODY] = 1200;
    unsigned before = f.decisions;
    dialog_fixture_frame(&f);
    assert(f.surface.overlay.open && f.decisions == before);
    dialog_equal(f.surface.body_scroll, 0, "reopen stale wheel");
    action = dialog_semantic(&f, 101);
    assert(action && !action->disabled);
    wheel(&f, -2);
    dialog_fixture_frame(&f);
    assert(f.surface.body_scroll > 0);
    dialog_fixture_pointer(&f, 101);
    dialog_fixture_frame(&f);
    assert(!f.surface.overlay.open && f.decisions == before + 1);
    /* Owned storage is removed only after the completed frame. */
    dialog_fixture_destroy(&f);
}
static void deferred_request(bool styled) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    f.composition.present = 1u << CUI_DIALOG_BODY;
    f.metric[CUI_DIALOG_BODY] = 500;
    dialog_fixture_frame(&f);
    cui_surface_request_close(f.ui, &f.surface, CUI_SURFACE_CANCELLED);
    assert(f.surface.overlay.open && f.closes == 0 && f.decisions == 0);
    dialog_fixture_frame(&f);
    assert(!f.surface.overlay.open && f.closes == 1 && f.decisions == 1);
    assert(f.closed_reason == CUI_SURFACE_CANCELLED && f.focus_closed == 1);
    dialog_fixture_destroy(&f);
}

static void held_across_reopen(bool styled, cui_surface_kind kind, cui_event down) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    cui_overlay_close(f.ui, &f.surface.overlay);
    cui_surface_init(&f.surface, 50, kind);
    assert(cui_dialog_layout_init_v1(&f.layout, &f.surface));
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    f.action_footer = true;
    f.composition.present = 1u << CUI_DIALOG_FOOTER;
    dialog_fixture_frame(&f);
    cui_rect bounds = cui_bounds(f.ui, 101);
    assert(bounds.w > 0 && bounds.h > 0);
    down.x = bounds.x + 2;
    down.y = bounds.y + 2;
    cui_event up = down;
    up.type = down.type == CUI_EVENT_KEY_DOWN ? CUI_EVENT_KEY_UP : CUI_EVENT_POINTER_UP;
    assert(cui_focused(f.ui) == 101);
    assert(cui_queue_event(f.ui, down));
    dialog_fixture_frame(&f);
    assert(cui_pressed(f.ui, 101) && f.actions == 0 && f.decisions == 0 && f.closes == 0);

    cui_overlay_close(f.ui, &f.surface.overlay);
    assert(cui_focused(f.ui) == 1);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    dialog_fixture_frame(&f);
    assert(cui_focused(f.ui) == 101 && !cui_pressed(f.ui, 101));
    assert(cui_queue_event(f.ui, up));
    dialog_fixture_frame(&f);
    assert(f.surface.overlay.open && f.actions == 0 && f.decisions == 0 && f.closes == 0 &&
           f.click_callbacks == 0);

    assert(cui_queue_event(f.ui, down));
    assert(cui_queue_event(f.ui, up));
    dialog_fixture_frame(&f);
    assert(!f.surface.overlay.open && f.actions == 1 && f.decisions == 1 && f.closes == 1);
    assert(f.closed_reason == CUI_SURFACE_ACCEPTED && f.focus_closed == 1);
    assert(f.click_callbacks == (styled ? 1u : 0u));
    dialog_fixture_destroy(&f);
}

static void direct_action_across_reopen(void) {
    dialog_fixture f;
    dialog_fixture_init(&f, true, NULL);
    f.action_footer = true;
    f.direct_footer = true;
    f.composition.present = 1u << CUI_DIALOG_FOOTER;
    dialog_fixture_frame(&f);
    dialog_fixture_pointer(&f, 101);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                             .accessibility_target = 101,
                                             .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    cui_overlay_close(f.ui, &f.surface.overlay);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    dialog_fixture_frame(&f);
    assert(f.actions == 0 && f.surface.overlay.open);
    dialog_fixture_pointer(&f, 101);
    dialog_fixture_frame(&f);
    assert(f.actions == 1 && f.surface.overlay.open);
    dialog_fixture_destroy(&f);
}

static void touch_scroll_across_reopen(void) {
    dialog_fixture f;
    dialog_fixture_init(&f, true, NULL);
    f.metric[CUI_DIALOG_BODY] = 800;
    f.composition.present = 1u << CUI_DIALOG_BODY;
    dialog_fixture_frame(&f);
    cui_rect body = f.layout.completed[5];
    assert(body.w > 30 && body.h > 64);
    cui_event down = {.type = CUI_EVENT_POINTER_DOWN,
                      .pointer_type = CUI_POINTER_TOUCH,
                      .pointer_id = 8,
                      .x = body.x + 24,
                      .y = body.y + 60};
    cui_event move = down;
    move.type = CUI_EVENT_POINTER_MOVE;
    move.y -= 48;
    cui_event up = move;
    up.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f.ui, down));
    dialog_fixture_frame(&f);
    cui_overlay_close(f.ui, &f.surface.overlay);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 1));
    dialog_fixture_frame(&f);
    assert(cui_queue_event(f.ui, move));
    dialog_fixture_frame(&f);
    assert(cui_queue_event(f.ui, up));
    dialog_fixture_frame(&f);
    dialog_equal(f.surface.body_scroll, 0, "old contact after reopen");
    assert(cui_queue_event(f.ui, down));
    assert(cui_queue_event(f.ui, move));
    assert(cui_queue_event(f.ui, up));
    dialog_fixture_frame(&f);
    dialog_equal(f.surface.body_scroll, 48, "new contact after reopen");
    dialog_fixture_destroy(&f);
}

int main(void) {
    actions_and_reopen(false);
    actions_and_reopen(true);
    deferred_request(false);
    deferred_request(true);
    direct_action_across_reopen();
    touch_scroll_across_reopen();
    const cui_event presses[] = {
        {.type = CUI_EVENT_POINTER_DOWN},
        {.type = CUI_EVENT_POINTER_DOWN, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 8},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE},
    };
    for (unsigned styled = 0; styled <= 1; ++styled)
        for (size_t i = 0; i < sizeof presses / sizeof presses[0]; ++i) {
            held_across_reopen(styled != 0, CUI_SURFACE_DIALOG, presses[i]);
            held_across_reopen(styled != 0, CUI_SURFACE_ALERT, presses[i]);
        }
    puts("Dialog owner actions/veto/focus/deferred close passed");
    return 0;
}
