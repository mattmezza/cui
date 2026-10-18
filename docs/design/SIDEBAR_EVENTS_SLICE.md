# Sidebar event checkpoint

Activation, item expansion and collapse Base/Styled slices implemented in this branch; see [Sidebar events](../SIDEBAR_EVENTS.md). Header/footer action forwarding is implemented in the final independent slice. Original audit: `2d7e16d1`. Activation, expansion and collapse payload/handler/result types are exposed now.
Preserve the shared component event contract, existing public layouts and all old
callbacks/results. This is an additive seam proposal, not new input routing.

## Existing source contract

`cui_sidebar.h` separates retained virtual layout/cache state from immediate menu
items, header/footer actions and a controlled collapse toggle. Entry render and
icon/label/suffix/context callbacks are composition, not behavioral events.
`cui_sidebar_prepare` reconciles bounded cache/motion; collapse options are
owner-supplied. Its successful bool is not an activation or value notification.

Base item validates ID/label/state, initializes uncontrolled open state, registers
TREEITEM and optional Button caret semantics, then loops existing inputs in index
order. Row invocation can open/toggle the submenu before old `on_activate`.
Caret invocation only changes expansion. `cui_sidebar_set_open` writes internal
`state.open` only when `expanded == NULL`, then invokes old `on_expand` in either
mode. With `expanded != NULL`, it never writes owner storage: old `on_expand` is a
request and the owner can accept/reject synchronously. The next event rereads the
actual owner; no optimistic request state is retained. Final result/semantics read
owner state again. `expansion_changed` records attempts, not owner acceptance.

The Styled menu traverses items/visible children in source order, reducing and
painting each item before the next. Thus callbacks across items are traversal
ordered, and local input indices can decrease. An observer must not sort or defer
this sequence. Custom rendering and context composition retain their old positions.

Base collapse toggle registers a Button, starts a local bool at options.collapsed,
and flips that local bool for each matching activation before old `on_change`.
This is a sequence of requests from by-value options, not applied owner writes;
callback-driven external updates do not replace the local sequence mid-call.
Styled toggle reduces once before paint. Header/footer actions use pointer-only
semantics, existing item-specific focus scope and old callback before content.
Their successful bool means valid submission, not action count.

## Proposed additive types and functions

New `cui_sidebar_events.h` includes Sidebar plus shared metadata:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_id item;
    unsigned level;
} cui_sidebar_activation;
typedef struct {
    cui_component_event_metadata event;
    cui_id item;
    unsigned level;
    bool previous, value;
} cui_sidebar_expansion_event;
typedef struct {
    void (*on_activate)(void *, const cui_sidebar_activation *);
    void (*on_expand_request)(void *, const cui_sidebar_expansion_event *);
    void (*on_expand_change)(void *, const cui_sidebar_expansion_event *);
    void *user;
} cui_sidebar_item_handler;
typedef struct {
    cui_sidebar_item_result control;
    size_t activation_count, request_count, change_count;
    bool valid, count_overflow;
} cui_sidebar_item_event_result;
```

First expose only the implemented subset of these fields. Add Base
`cui_base_sidebar_item_events(ui,item,collapsed,level,handler)` and Styled
`cui_sidebar_menu_events(ui,id,theme,items,count,collapsed,handler)`. Menu result
has legacy submission bool plus actual callback counts and validity; it must not
pretend to return one item's final open state. Base event.target is item ID;
Styled menu uses compound menu ID, preserving original copied activation.target.
Payload item/level identifies the actual affected row. Source array index is not
needed as identity. No extra workspace or retained handler.

For toggle, propose distinct typed collapse request `{event,previous,value}` and
handler/user plus result `{activated,request_count,count_overflow,valid}` on
`cui_base_sidebar_toggle_events` and `cui_sidebar_toggle_button_events`, with
existing arguments plus handler. Collapse is not expansion: retain separate names.
Header/footer additive entrypoints use the action payload/count convention with
existing options plus action handler; do not expose unimplemented fields early.

## Narrow timing and ownership seams

1. **Item activation first.** Freeze accepted activation evidence at the existing
   row branch before expansion/old callbacks. Invoke typed ACTIVATE after the old
   `on_activate`, preserving expansion-before-activation ordering. Repeated same
   row invocations all notify, including when old callback is absent. No action
   for caret-only toggle, passive/disabled/inert work or successful paint alone.
2. **Item expansion separately.** Capture actual before and requested next plus
   cause before the old set-open operation. Run its write/old callback once.
   Controlled mode then invokes typed REQUEST, allowing that specific handler to
   assign the same controlled expanded bool synchronously. No input-consumption,
   propagation or default-prevention decision follows from that assignment.
   Uncontrolled mode instead reads actual state.open after old callback, invokes
   typed CHANGE only if before differs, and forbids observer model mutation.
   Do not label a controlled request an applied change, even if an earlier legacy
   callback already accepted it. Do not emit initialization/default maintenance.
   Old callback -> typed expansion -> old activation -> typed activation remains
   synchronous. Capture evidence once and reuse it across related notifications.
3. **Collapse toggle separately.** Preserve the exact old local flip sequence;
   old on_change then typed REQUEST for each actual activation. Handler may update
   unrelated owner state/options for the next submission, but cannot mutate the
   active copied options or infer confirmed state. Missing callbacks preserve the
   old reducer/result. No after-frame dispatch or controlled-state architecture.
4. **Header/footer separately.** Existing old action then typed ACTIVATE before
   content rendering, pointer-only semantics unchanged. A typed handler may act
   without an old callback on the additive entrypoint; old entrypoint remains
   byte/behavior equivalent. No keyboard/accessibility support is invented.

Try actual Core accepted snapshots while current semantics already exist; preserve
original target/frame/index/source. If unavailable, carry only proven actual local
index/type/frame. TEXT never implies keyboard. No early interact, extra reduction,
synthetic activation or global capture/bubble. Input consumed/default state stays
unchanged. Menu validation/recursive traversal and final semantics/paint remain
at their existing positions. Callback-visible arrays/geometry may be intermediate.

All new handler/user/payload pointers are borrowed during submission. Pointer-free
payload copies survive omission and owner teardown; handles do not own live rows.
Only the controlled REQUEST handler may update the explicit expanded bool under
its documented narrow permission. Otherwise no active item arrays/labels/child
state/userdata/input mutation, recursive UI, context destruction or storage removal.
Legacy callback mutation behavior is retained. Defer structural changes and
resource teardown until outer return and renderer consumption. Retained Sidebar
entry/cache/header/footer content lifetimes are unchanged by immediate observers.

## Independent evidence and binding omissions

Keep `test_sidebar.c` and Settings regressions: Settings uses these existing
callbacks and must not acquire duplicate actions. Add dedicated headless tests:
old/new results, state, callback trace, focus, semantics and draw equivalence;
repeated/net-zero inputs; old and typed handlers coexisting; request acceptance,
rejection and redirection separately from consumption; internal open-state legacy
redirection; default/disabled/inert/passive/collapsed/disable-caret policy; item
traversal with decreasing input indices; copied metadata/lifetime/deferred removal.
Exercise actual pointer/keyboard/accessibility paths only where Core supports them;
TREEITEM role alone does not prove accessibility ACTIVATE. Header/footer remain
pointer-only. Toggle tests must preserve by-value local request alternation.
Strict C11/C++ headers, warning builds and private-Xvfb native tests are required.

Bridge Sidebar has its own retained menu/control state and queued callbacks. No
Python/JS/TS/Go/Rust builder automatically acquires immediate typed requests,
per-item metadata or synchronous acceptance from these C facades. Existing generic
queue timing, copies and lifecycle remain separate, with explicit omissions.
No source manifest promotion or new dependency follows from this checkpoint.

## Activation implementation checkpoint

The shipped activation names match this proposal. The menu result is named
`cui_sidebar_menu_event_result`; its `submitted` field is the old submission bool,
not an aggregate action. `valid` records argument/tree validation. Header discovery
includes the new standalone `cui_sidebar_events.h`; old public layouts are unchanged.
Actual snapshots are copied before legacy callbacks. A tested accessibility-focus
then Enter sequence lacks a current accepted snapshot, so only genuine keyboard,
frame and index metadata is carried there. TREEITEM ACTIVATE is rejected at ingress.
Generic consumption of later ordinary mouse events does not cancel cached activation;
explicit cancellation does. Both legacy outcomes remain unchanged. These are existing
Core facts, not new acceptance/propagation rules. Expansion fields have now been appended to the unreleased handler/results as approved; rebuild consumers. Section event fields remain unexposed.

## Expansion implementation checkpoint

Controlled REQUEST runs after the legacy on_expand with frozen previous/requested
values, regardless of earlier acceptance; only its explicit owner bool may be
assigned. Uncontrolled CHANGE reads actual state.open after the legacy callback,
suppressing redirected net-zero transitions. Existing attempt flags remain unchanged.
Shared evidence is frozen before any legacy callback and reused for related row
activation; caret evidence retains its actual Button identity. Tests prove both
controlled acceptance paths/rejection/repeated requests, precise synchronous order,
legacy cancellation after capture, missing observers, caret accessibility/touch,
passive work and Styled compound forwarding. This is an unreleased struct extension,
not an ABI-preserving change to a published event handler. Legacy Sidebar layouts
are unchanged; header/footer remain a separate review checkpoint.

## Collapse implementation checkpoint

Separate `cui_sidebar_collapse_request`/`cui_sidebar_collapse_handler` and
`cui_sidebar_toggle_event_result` support Base `cui_base_sidebar_toggle_events` and
Styled `cui_sidebar_toggle_button_events`. Requests follow the legacy local flip
sequence from entry options, old callback then typed callback per activation.
External owner assignment is permitted but never redirects that local sequence.
Semantics and Styled paint retain the entry snapshot, including after acceptance.
No collapse CHANGE, callback reconstruction from aggregate bool or hidden resync.
Bounded tests compare actual old/new callback values and Styled geometry, repeated
requests, divergent owner writes, entry semantics, genuine pointer/touch/key/Button
accessibility, inert/layout-only/disabled/missing handlers and copied lifetime.
Header/footer were subsequently approved and implemented; see the routing correction below.

## Header/footer routing correction and implementation

The earlier pointer-only description was incomplete: existing GROUP sections set
`pointer_activatable`, which Core's accessibility eligibility explicitly accepts.
The additive Styled-only `cui_sidebar_header_events` / `cui_sidebar_footer_events`
therefore preserve pointer, touch and accepted accessibility ACTIVATE. No keyboard
activation routing or Base section API is introduced. Shared activation handlers
and action results report actual observer calls, after each legacy callback;
metadata is frozen beforehand. Legacy entrypoints retain their original path.
