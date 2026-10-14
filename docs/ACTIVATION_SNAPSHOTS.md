# Copied activation snapshots

`cui_event_activation_snapshot` returns copied evidence for an activation already accepted by a submitted control. Query it after `cui_interact` accepts the activation, during ordinary frame composition. Each accepted event retains its own slot even when a frame contains multiple clicks or synthesized touch cancellation reorders the dispatch list.

```c
cui_activation_snapshot click;
if (cui_event_activation_snapshot(ui, event_index, &click)) {
    if (click.has_press && click.has_release) {
        float dx = click.release.x - click.press.x;
        (void)dx;
    }
    if (click.has_key_bounds) {
        cui_rect anchor = click.key_bounds;
        (void)anchor;
    }
}
```

The context stores a bounded snapshot beside each existing activation slot. It performs no additional per-event allocation. A pointer press is retained across frames only while the existing primary capture survives. Cancellation, touch gesture takeover, removal and disabled capture clear the retained press. Leaving the window preserves it. A release must match pointer type, contact, device and button before `has_press` becomes true. Existing routing can accept some unmatched releases; those retain release evidence with `has_press=false` instead of borrowing another contact's press.

Keyboard evidence contains the previous rendered clipped routing rectangle, even if the current composition moves the control. Accessibility activation has no fabricated press, release or keyboard bounds. Raw pointer click count zero remains zero. First-focusing-click and long-press metadata require separate authoritative host support and are not exposed here.

False leaves the output untouched. The query refuses unaccepted/cancelled events, current disabled/modal/accessibility rejection, invalid indices, null output, Core errors, inert/layout-only ancestry, measurement/prepaint and calls outside a frame. Accessibility consumption can invalidate a subsequent query; copy evidence before callbacks that may consume it. A returned value contains no pointers and survives later frames or context destruction. Frame sequence and event index identify only this context/frame, not recreated contexts or sequence wrap.

The default retains Enter-down, Space-up, focus and consumption behavior.
Core now also supports opt-in activation on key release; see
[the bounded release-key policy](validation/release-key-activation-sol6.md).
Button forwards copied evidence before callbacks; see
[Button click evidence](BUTTON_CLICK_EVIDENCE.md). Signed tab paths and
capture/bubble propagation remain separate unfinished capabilities.

Standalone Core-only headless tests cover multiple clicks, cross-frame pen capture and leave/enter, raw counts/modifiers/device, unmatched releases, cancellation, previous rendered keyboard bounds, modal/disabled/accessibility rejection, phase guards, output preservation, retained-copy lifetime and synthesized touch event reordering. Native strict C11 and ASan/UBSan/leak checks pass. Actual Zig0.16/Wasmtime49 reports136-byte snapshots and40-byte pointer samples, matching this native host; these are receipts rather than cross-platform ABI requirements. Portable full CTest148/148 passes; the final expanded fixture was rerun separately. WASI assertions remain enabled.
