# Sidebar row activation events

This additive C slice provides Base and Styled **row activation notifications**.
Expansion requests/changes, collapse requests and header/footer actions remain later
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
        (cui_sidebar_item_handler){opened, last_item});
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
an activation. `activation_count` counts actual typed observer calls, zero with a
missing observer. Saturating counts expose `count_overflow`.

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

Handler, user and payload pointers are borrowed only during submission. Copying the
pointer-free payload preserves it through later frames, omission and context teardown;
IDs do not retain live rows. Typed observers may update unrelated application outputs,
but must not mutate active item arrays/labels/child state/input storage, recurse into
UI composition, remove active storage or destroy the context. Context destruction
during dispatch remains unsupported; there is no fake teardown guard. Defer structural
changes and resource teardown until outer submission and renderer consumption finish.

`tests/unit/test_sidebar_events.c` proves repeated/net-zero Base activations,
legacy expansion/activation/typed order, old/new results and callback counts, pointer,
touch, keyboard, rejected accessibility activation, focus+keyboard, missing handlers,
legacy state redirection, caret exclusion, disabled/passive/omitted paths, nested
levels, traversal with decreasing indices, preserved consume/cancel behavior and
copied lifetime. Styled legacy/new paint command types/bounds/colors are compared;
this is scoped geometry/paint evidence, not full renderer parity. Existing Sidebar
and Settings tests and strict C++17 header compilation also pass. Native CTest runs
only through `python scripts/test_local.py --build-dir <external-build>` (private Xvfb).

Bridge Sidebar has separate retained state and queued generic notifications. No
Python/JS/TS/Go/Rust API automatically gains these immediate per-item notifications
or future synchronous expansion requests. SDK/module/export wiring is integrator work.
