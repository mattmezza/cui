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
    wheel(&f, -2);
    dialog_fixture_frame(&f);
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
int main(void) {
    actions_and_reopen(false);
    actions_and_reopen(true);
    deferred_request(false);
    deferred_request(true);
    puts("Dialog owner actions/veto/focus/deferred close passed");
    return 0;
}
