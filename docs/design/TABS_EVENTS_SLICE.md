# Tabs selection observer proposal

Design-only audit at `54296dbe`; no API below is implemented. Preserve the legacy
Base/Styled bool APIs and the 78-family manifest. No full Tabs/binding parity claim.

## Actual source contract

[`cui_tabs_options`](../../include/cui/cui_choice.h) combines disabled/manual
activation, optional scroll/overflow/reorder state, render slots, panel callbacks
and a retained indicator. Selected value is a caller-owned stable ID: zero means
none; invalid/disabled selections are not repaired and produce no visible panel.
There is no selection observer in these options. Render/panel callbacks are UI
composition, not notifications. Reorder `requested/moved/before` is a separate
owner-applied request; it is not selection CHANGE.

[`cui_base_tabs`](../../src/base/cui_choice.c) performs these passes:

1. Validate shadow policy; clean up empty overflow and drag state.
2. Reduce viewport/scrollbar input, possibly suppress pointer selection against
   stale pre-scroll layout; scrolling also cancels drag candidates.
3. Reduce overflow MenuTree, map its action ID back to a tab ID, and assign selected.
4. Register TAB semantics, then process remaining unconsumed events. Activation
   assigns selected; navigation moves focus and, unless manual_activation, assigns
   selected. Disabled items/global disabled suppress this selection loop.
5. Update selected semantics, panel controls and roving tab stop. Return whether
   the selection at the pre-overflow snapshot differs from final selected.

This is multiple passes, not a single globally chronological selection loop.
Overflow returns one selected action, so the outer Tabs call does not possess a
unique original event index for every selection it can apply. Same-tab activation
is not CHANGE. Several selections can coalesce or return to the initial ID.

[`cui_tabs`](../../src/components/cui_choice.c) resolves metrics/theme/geometry,
calls Base once, updates reveal/scroll/animation state, paints slots/overflow,
then submits the selected enabled panel and returns Base's bool. Indicator bounds
are written by end_frame and animation state must survive that boundary. The
panel callback runs after Base selection finalization. Existing custom close-tab
examples defer applying requested removals; do not mutate the item array in a
notification or render callback.

Evidence is concentrated in [`test_choice.c`](../../tests/unit/test_choice.c):
`test_tab_overflow_menu`, `test_tab_drag_reorder`, `test_tab_scrolling`,
`test_tab_reveal_requests`, `test_tab_scrollbars`, `test_tabs_toolbar_consumption`,
`test_tab_current_frame_baseline`, and the final Base manual-activation assertions.
These are existing behavior evidence, not proof of a new facade.

## Recommended first slice: explicit aggregate CHANGE

Use a new `cui_tabs_events.h`, including `cui_choice.h` and
`cui_component_events.h`; do not introduce a Core dependency on components.
The compound ID is explicit for Base because the legacy Base API has none:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_id previous, value; /* Stable selected IDs; no borrowed item data. */
} cui_tabs_selection_change;
typedef void (*cui_tabs_change_fn)(void *, const cui_tabs_selection_change *);
typedef struct { cui_tabs_change_fn on_change; void *user; } cui_tabs_handler;
typedef struct {
    bool changed;        /* Exact legacy bool result. */
    size_t change_count; /* Zero or one actual observer invocation. */
} cui_tabs_event_result;

cui_tabs_event_result cui_base_tabs_events(
    cui_context *, cui_id target, const cui_choice_item *, size_t count,
    cui_id *selected, cui_tabs_options, cui_tabs_handler);
cui_tabs_event_result cui_tabs_events(
    cui_context *, cui_id container, const cui_theme *, const cui_choice_item *,
    size_t count, cui_id *selected, cui_tabs_options, cui_tab_panel_fn, void *panel_user,
    cui_tabs_handler);
```

Dispatch at most one applied selection CHANGE after Base completes semantic/roving
focus finalization, before Styled paint/slots/panel. Private common Styled
implementation calls the Base facade at its existing seam; legacy path remains
unchanged. Capture selected before/after one Base invocation. Do not call legacy
Styled and Base again, and do not observe arbitrary subsequent mutations made by
custom panel/render code as if they were Tabs input. Keep legacy return flags
verbatim and do not invent valid/submitted flags from the bool API.

A missing observer still permits all legacy writes, producing zero callback count.
If final selected equals initial selected, emit nothing even if intermediate IDs
changed. Payload has CHANGE, compound target, UNKNOWN source, actual nonzero frame
sequence with HAS_FRAME only; no index, item cause, activation or host timestamp.
Unlike [`cui_selection_change`](../../include/cui/cui_selection_events.h), it does
not assert ACTIVATION/NAVIGATION reason when overflow/multiple passes make that
unknown. New typed fields are pointer-free and copyable. No REQUEST, COMMIT,
ACTIVATE, reorder or popup lifecycle event is implied.

Require a valid nonzero compound target for the new Base facade (the caller chooses
it); Styled uses container. New admission checks must precede legacy cleanup if
rejection promises atomicity, without changing old functions' admission behavior.
The old false result conflates no change and some invalid inputs; document this
instead of retroactively defining a successful submission signal.

## Routing, accessibility, disabled and lifetime boundaries

Keep Core target/activation/consumed-input routing and the existing semantic TAB
roles, roving focus, panel controls/labelled_by IDs and overflow MenuTree behavior.
Focus-only manual navigation, scrolling, drag requests and popup open/close do not
emit selection CHANGE. Do not synthesize activation from a final selected value.
Accessibility only contributes through actual routed Core actions; no new host
accessibility support is implied. Current-frame routes/previous layout remain the
existing behavior. Evidence/index omission is deliberate, not lost metadata that
can be reconstructed from consumed events.

There is no Tabs readonly/inert option. Disabled item/global disabled behavior must
match legacy exactly, including pre-loop cleanup. Ancestor inert behavior belongs
to Core routing, not a new facade suppression flag; test it explicitly rather than
promise that all direct reducer state maintenance stops in an inert subtree.
Do not bypass modal scopes or unconsume rejected input. Selection writes are
post-mutation notifications and cannot be rejected by the observer.

Handler/user are submission-borrowed. Preserve item/selected/options/state storage,
render users, overflow scratch and panel resources throughout the call. Observer
must not change/free selected, arrays or active state, destroy context, or reenter
UI. Unrelated application flags may change; defer removal/reorder/teardown until
return plus frame/render consumption. Retained animation/resource users keep their
existing longer lifetime. New observer omission is not cancellation of retained
work. Legacy render callback order remains intact.

## Alternative and verification boundary

A per-transition variant could hook direct activation/navigation assignment sites
with genuine event indices, but overflow would need its own indexed action seam
and a reviewed ordering contract. It cannot honestly promise one chronological
stream by adding an observer to the final bool result. Defer that larger slice;
do not silently upgrade the aggregate API later to multiple callbacks.

Proposed ownership: new header, narrow Base wrapper/common Styled seam in
`src/base/cui_choice.c` and `src/components/cui_choice.c`, discoverability comment
in `include/cui/cui_choice.h`, dedicated `tests/unit/test_tabs_events.c` and guide.
Root owns registration, exports and generated SDKs. Tests must compare old/new
selected/focus/semantics/panels/draw commands for pointer and keyboard activation,
manual focus-only navigation, same-tab no-change, multiple events including
return-to-initial, disabled/invalid/empty selections, overflow selection, drag and
scroll silence, consumed toolbar input, inert/modal routing, absent observer,
callback-before-panel order, copied payload lifetime and deferred close/removal.
Keep existing choice tests; native checks use the private-display wrapper and
browser checks remain headless. No builds were run for this source/design audit.

Foreign Python/JS/TS/Go/Rust V1 notification views do not automatically expose this
new C proposal. Their queues retain post-frame timing and explicit payload
allowlists; see [binding audit](COMPONENT_EVENTS_BINDINGS.md). Typed Tabs selection
would require a separately owned queue/schema/wrapper slice with stable full-width
IDs, bounded ownership and no synchronous owner-rejection claim.

## Implementation status

The approved aggregate slice is implemented in `cui_tabs_events.h`, with a whole
Base wrapper in `src/base/cui_tabs_events.c` and the narrow Styled Base-call seam.
See [the implementation guide](../TABS_EVENTS.md). Legacy maintenance still runs;
notification is restricted to an active painted, non-disabled selection transition
with a proven nonzero frame. Inert routing produces no synthetic selection event.
This does not implement the per-transition alternative or foreign wrappers, and
no parity status is promoted.
