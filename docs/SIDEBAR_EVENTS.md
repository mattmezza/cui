# Sidebar row activation events

This additive C slice provides Base and Styled **row activation notifications**
and typed item expansion and collapse requests. Header/footer actions remain later
slices. Legacy Sidebar callbacks, results, layouts and retained cache lifetimes are
unchanged; no component parity or binding coverage is promoted.

```c
#include <cui/cui_sidebar_events.h>
static void opened(void *user, const cui_sidebar_activation *event) {
    cui_id *last_item = user;
    *last_item = event->item; /* Application-owned output, not active item storage. */
}
static void render_menu(cui_context *ui, const cui_theme *theme,
                        const cui_sidebar_menu_item *items, size_t count,
                        cui_id *last_item) {
    cui_sidebar_menu_event_result result = cui_sidebar_menu_events(
        ui, 100, theme, items, count, false,
        (cui_sidebar_item_handler){.on_activate = opened, .user = last_item});
    (void)result;
}
```

Call the render helper during normal frame composition. The existing
`cui_sidebar.h` also exposes the new declarations. The Base entrypoint is
`cui_base_sidebar_item_events(ui, item, collapsed, level, handler)` and returns
`cui_sidebar_item_event_result`, containing the unchanged legacy `control` result.
The Styled menu returns `cui_sidebar_menu_event_result`, not one row's open state.
`valid` means argument/tree validation succeeded. Styled `submitted` is the old
menu submission bool, including inert/layout-only composition; neither flag implies
an activation. `activation_count`, `request_count` and `change_count` count actual typed observer
calls, zero for each missing observer. Saturating counts expose `count_overflow`.

Each `cui_sidebar_activation` contains pointer-free common metadata, actual `item`
ID and nesting `level`. Its kind is ACTIVATE, including repeated invocations of the
same row. Base metadata targets the row; Styled metadata targets the public menu ID.
A copied activation snapshot, when available, retains its original row target.
Do not sort by event index: Styled reduces/paints in item traversal order, so input
indices can decrease across rows. Every actual row invocation notifies; there is no
aggregate-last reconstruction or after-frame dispatch.

Evidence is frozen at the existing row branch **before** expansion or legacy
callbacks. Existing expansion write/request and `on_expand` run first, then existing
`on_activate`, then the typed observer synchronously, before final semantics/paint.
There is no second reduction, early semantic registration, acceptance decision or
observer veto. Legacy owner redirection of uncontrolled state is preserved. A missing
old activation callback does not prevent typed notification; a missing typed handler
does not prevent normal legacy mutation. Caret-only expansion is not row activation.
Defaults/initialization, disabled rows, inert/layout-only work and omitted children
do not generate activation notifications.

Real pointer, touch and keyboard paths use genuine accepted snapshots when available.
The existing Core has a tested accessibility-focus/Enter transition where legacy row
activation exists but a copied accepted snapshot is unavailable. The fallback carries
only genuine local frame/index and explicit input source; HAS_ACTIVATION remains
absent. Presence flags alone establish evidence; empty snapshot fields prove nothing.
TREEITEM accessibility **ACTIVATE is unsupported** and rejected at queue ingress.
Accessibility FOCUS followed by keyboard activation is tested separately; this does
not establish accessibility activation parity. No Core allowlist or routing changes.

Existing callback input mutation remains existing behavior. In particular, consuming
later ordinary mouse input does not cancel its cached activation at this seam;
explicit `cui_cancel_event_activation` does. Tests preserve both outcomes. The new
observer does not reinterpret consumption as a notification veto.

## Expansion ownership and order

Set `on_expand_request` and/or `on_expand_change` on `cui_sidebar_item_handler`.
Both receive `const cui_sidebar_expansion_event *` containing copied metadata,
actual item ID/level and `previous`/`value` booleans. These handler and result fields
extend an **unreleased branch API**: rebuild consumers together with the library.
Zero and designated initializers remain source compatible; legacy Sidebar public
layouts are unchanged.

With `item.expanded != NULL`, the existing operation remains a controlled request.
The legacy `on_expand` runs first, followed by typed REQUEST with the frozen previous
owner value and requested next value. The component never assigns owner storage.
The typed request handler may synchronously assign **that explicit expanded bool**;
rejection simply leaves it unchanged. Each later input rereads actual owner state.
Repeated rejected toggles therefore request the same next value; accepted toggles
alternate. Even if the legacy callback already accepts, the typed event remains a
request, not a confirmed CHANGE. A click-to-open attempt already at open does not
invoke either old or typed expansion callbacks.

With `expanded == NULL`, the existing state write and old callback happen once.
Typed CHANGE reports the actual final `state.open`, only if it differs from the
frozen previous state. Legacy redirection back to the previous value emits no CHANGE,
even though the old `expansion_changed` attempt flag remains true. Initialization,
owner-only programmatic changes and maintenance do not notify. The typed CHANGE
observer cannot mutate active state.

For a row that expands, order is old expansion → typed expansion → old activation →
typed activation. Caret-only expansion invokes no row activation observer. Metadata
is frozen before old callbacks, even if they cancel the current input afterward.
Base expansion target is the item ID and Styled target is the menu ID; copied
activation.target remains the actual row or caret Button. Caret accessibility
ACTIVATE is genuinely supported and tested separately from unsupported TREEITEM
activation. Existing `disable_caret`, collapsed/omitted and passive policies remain;
a disabled row does not implicitly disable its separate caret in the legacy API.

## Collapse toggle requests

`cui_base_sidebar_toggle_events(ui, id, options, handler)` and
`cui_sidebar_toggle_button_events(ui, id, theme, options, handler)` return
`cui_sidebar_toggle_event_result`. Its `activated` is the unchanged legacy toggle
bool; `valid` records argument validation. `request_count` counts actual typed calls
and `count_overflow` reports saturation. A valid inert/layout-only submission has
no activation or request count. A missing handler still permits normal legacy work.

Use a separate `cui_sidebar_collapse_handler` with `on_request`/`user`. The callback
receives `cui_sidebar_collapse_request { event, previous, value }` with REQUEST
metadata targeting the public toggle ID. For example:

```c
static void collapse_requested(void *user, const cui_sidebar_collapse_request *request) {
    bool *owner_collapsed = user;
    *owner_collapsed = request->value; /* Optional acceptance of the request. */
}
```

The reducer starts a local bool at the entry `options.collapsed`, then flips that
local bool for each existing activation. With entry false and three inputs, requests
are `(false,true)`, `(true,false)`, `(false,true)`. The old `on_change` runs before
the typed request for each activation. Both callbacks may assign an external owner
collapsed bool, but **neither assignment redirects the active local flip sequence**.
The caller supplies an updated owner value in a later submission. These are requests,
not notifications of confirmed owner changes; no collapse CHANGE is emitted.

Genuine metadata is captured before the old callback. The original semantics
`expanded = !options.collapsed` and the Styled icon/paint also retain the entry
snapshot for that submission, even after an owner accepts or rejects. No silent
semantic/visual resynchronization, second reduction or aggregate reconstruction is
introduced. The narrow permitted typed mutation is assignment of the external
collapsed owner bool; active options/input/item storage and context teardown remain
unsupported. Handler/payload copies have the same lifetime rules below.

Tests compare old/new local value sequences, aggregate bool and Styled command
geometry/colors; prove legacy-then-typed ordering despite divergent owner writes,
entry-snapshot semantics, repeated requests, pointer/touch/keyboard/genuine Button
accessibility, missing handlers, disabled/inert/layout-only work, invalid arguments
frozen current evidence after legacy cancellation of remaining inputs, and copied metadata after context destruction. Header/footer observers remain a
separate review slice.

Handler, user and payload pointers are borrowed only during submission. Copying the
pointer-free payload preserves it through later frames, omission and context teardown;
IDs do not retain live rows. Typed observers may update unrelated application outputs,
but, apart from the controlled expansion/collapse REQUEST assignments above, must not mutate active item arrays/labels/child state/input storage, recurse into
UI composition, remove active storage or destroy the context. Context destruction
during dispatch remains unsupported; there is no fake teardown guard. Defer structural
changes and resource teardown until outer submission and renderer consumption finish.

`tests/unit/test_sidebar_events.c` proves repeated/net-zero Base activations,
legacy expansion/activation/typed order, old/new results and callback counts, pointer,
touch, keyboard, rejected accessibility activation, focus+keyboard, missing handlers,
legacy state redirection, caret exclusion, disabled/passive/omitted paths, nested
levels, traversal with decreasing indices, preserved consume/cancel behavior and
copied lifetime. Expansion tests cover controlled rejection/new-handler acceptance/legacy acceptance, repeated requests, actual changes and redirected net-zero suppression, precise callback order, frozen evidence after legacy cancellation, missing observers, passive work, genuine caret accessibility/touch and Styled forwarding. Styled legacy/new paint command types/bounds/colors are compared;
this is scoped geometry/paint evidence, not full renderer parity. Existing Sidebar
and Settings tests and strict C++17 header compilation also pass. Native CTest runs
only through `python scripts/test_local.py --build-dir <external-build>` (private Xvfb).

Bridge Sidebar has separate retained state and queued generic notifications. No
Python/JS/TS/Go/Rust API automatically gains these immediate per-item notifications
or synchronous expansion requests. SDK/module/export wiring is integrator work.
