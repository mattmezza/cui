# Disclosure applied event proposal

Design-only audit at `52b10b90`. No proposed facade below exists. Keep existing
Accordion/Collapsible APIs and all 78 manifest entries; no parity promotion.

## Actual contract and nonuniform results

[`cui_disclosure.h`](../../include/cui/cui_disclosure.h) gives callers ownership of
expanded bool storage, item arrays and optional stable per-panel motion arrays.
Item and descendant IDs must be unique, nonzero and stable. Header and panel
callbacks render arbitrary UI; they are not change handlers. No current typed
expanded-state observer exists.

[`cui_base_collapsible`](../../src/base/cui_disclosure.c) registers a button with
expanded/controls semantics, processes matching activations by toggling `*open`,
then updates semantics. Its bool means a toggle occurred, not final inequality:
two activations may return true with unchanged final state.

`cui_base_accordion` registers all triggers, then handles activation/navigation.
Opening a closed item toggles it; closing an open item requires collapsible=true.
Opening with multiple=false first clears **every** open slot, including disabled
siblings, then opens the chosen slot. Thus disabled means that item's direct input
is suppressed; it does not guarantee its expanded value cannot be closed by
single-open policy. Multiple=true leaves siblings untouched. The reducer does not
normalize arbitrary preexisting multi-open arrays on idle. Up/Down/Home/End move
focus among enabled triggers without changing expanded state. It updates all
trigger semantics at the end and returns whether any accepted toggle occurred.
The outer bool cannot identify changed panels, multiplicity, input or final delta.

[`cui_collapsible_ex` and `cui_accordion`](../../src/components/cui_disclosure.c)
call Base once, then render headers/panels with the resulting bools. The simple
`cui_collapsible` delegates to `_ex`. Without motion, closed panels are omitted.
With motion, callbacks continue during exit; closed content is inert, and progress
zero uses layout-only measurement. Motion measured bounds are written at end_frame,
so storage must outlive submission. Progress/velocity, initial/reduced-motion snaps
and panel measurement are presentation, not applied expanded CHANGE or lifecycle
completion. Omission does not execute cleanup or cancel retained measurement work.

Existing evidence: [`test_disclosure.c`](../../tests/unit/test_disclosure.c), notably
`test_custom_disclosure_headers`, `test_disclosure_box_overrides`,
`test_measured_reveal`, `test_accordion_reveal_storage`,
`test_configured_disclosure_springs`, and main's single/multiple/collapsible,
disabled, keyboard-focus and panel-count assertions. These tests prove old behavior,
not new dispatch.

## SUPERSEDED: initial aggregate workspace option (comparison only)

Start with aggregate final-delta notifications. Use a new `cui_disclosure_events.h`
including the existing disclosure and metadata headers:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_id item;
    size_t item_index; /* Model index, NOT a frame event index. */
    bool previous, expanded;
} cui_disclosure_change;
typedef void (*cui_disclosure_change_fn)(void *, const cui_disclosure_change *);
typedef struct { cui_disclosure_change_fn on_change; void *user; } cui_disclosure_handler;
typedef struct { bool changed; size_t change_count; } cui_disclosure_event_result;
```

`changed` preserves the exact old bool. Counts are actual callback invocations;
missing observer still edits and yields zero count. No rejection/REQUEST/COMMIT or
animation-completion contract is added. Event target is the compound identity;
item is the affected trigger, not a fabricated activating cause. Metadata is
CHANGE, UNKNOWN source, proven nonzero active frame with HAS_FRAME only. Payloads
are pointer-free/copyable, with no input index/activation or strings.

1. **Collapsible, independently first.** Proposed
   `cui_base_collapsible_events(ui,id,label,open,disabled,handler)` and
   `cui_collapsible_events(ui,id,theme,label,open,options,panel,panel_user,handler)`.
   The latter mirrors `_ex`, not the convenience bool-disabled signature. Return
   the new result. Capture one bool before Base, invoke it once, finalize then
   notify zero/one on final inequality. Item/target=id and item_index=0. Styled
   uses a narrow private common implementation at its existing Base seam, before
   render callbacks/panel; legacy path unchanged. Two toggles can leave legacy
   changed=true with callback count zero. No additional persistent state required.
2. **Accordion, separate review because snapshot ownership matters.** Propose a
   caller-provided temporary snapshot descriptor, avoiding hidden allocation or a
   new arbitrary family-size cap:

```c
typedef struct { bool *before; size_t capacity; } cui_disclosure_event_workspace;
cui_disclosure_event_result cui_base_accordion_events(
    cui_context *, cui_id target, const cui_disclosure_item *, size_t count,
    bool *open, cui_accordion_options, cui_disclosure_event_workspace, cui_disclosure_handler);
cui_disclosure_event_result cui_accordion_events(
    cui_context *, cui_id container, const cui_theme *, const cui_disclosure_item *, size_t count,
    bool *open, cui_accordion_options, cui_disclosure_panel_fn, void *panel_user,
    cui_disclosure_event_workspace, cui_disclosure_handler);
```

For an installed observer and nonzero count, workspace must have capacity>=count,
be writable and disjoint from open/items/state/resources. Copy bool values before
one Base call; afterward dispatch once per final changed slot, in **model index
order**, after all group semantics are finalized and before Styled rendering.
Single-open side-effect closes are included even for disabled siblings. This is
not chronological input order. Net-zero slots do not notify, and one activation
can yield several panel callbacks. No handler needs no snapshot/workspace; unused
user/workspace is ignored. Do not store workspace or borrowed payload pointers.
Required pointers/nonzero compound target and installed-handler workspace capacity
must be admitted before mutation; invalid admission has no callbacks or writes.
Legacy empty-array/admission behavior must remain intact in old functions.

This workspace is a real API tradeoff requiring integrator acceptance, not an
implicit authorization to allocate. An alternative per-transition hook inside the
Base assignment loops avoids whole-array snapshots but changes multiplicity and
callback timing: a listener may see intermediate group state, and automatic sibling
closes need distinct item/cause identity. It requires deliberate reducer refactoring
and evidence rules. A whole-call wrapper without any snapshot cannot recover the
previous bool for arbitrary count; do not fake it from final state or the old bool.
Do not choose a fixed small array and silently truncate notifications.

## Policy, routing and lifetime constraints

Keep disabled/direct-input, single-open side effects, custom header controls,
roving focus, controls IDs and input routing unchanged. Inert parent routing must
remain Core-owned. Layout-only/disabled/passive maintenance may still run, but
new observers should require active painted work, proven frame and global enabled
state. Do not filter a disabled sibling's actual policy-driven close just because
its own direct input was disabled. No readonly option exists, and no new
propagation/consumption guarantee is implied by adding observers. Accessibility
facts may only come from actual routed Core activation; aggregate events do not
manufacture an activation snapshot.

Caller must preserve arrays, bools, styles, motion slots, measured bounds,
header/panel/observer users and context. Observer must not mutate/free/rebind these
resources or recursively submit UI. Render callbacks retain old order; under the
new facade they must not mutate the active model or snapshot while rendering.
Unrelated application flags may change; defer omission/removal/reorder/teardown
until outer return plus frame/render consumption, and honor motion's end_frame
storage requirement. Reading results does not dispatch. Existing render callbacks
are not transformed into change callbacks and closing animation is not a late
expanded-state notification.

## Meaningful verification and binding omissions

Use paired old/new contexts and real queued pointer/key actions. Compare expanded
arrays, exact bool result, focus, semantics, header/panel ordering, draw commands
and retained motion/measurement state. Test two toggles/net-zero; multiple final
changes; single-open sibling close including disabled-open sibling; collapsible
false rejection; multiple=true preservation; navigation-only silence; custom
interactive header children; consumed/inert/modal/layout/disabled paths; existing
invalid multi-open model unchanged on idle; missing handler edits; copied payload
through later frames/destruction; callback-before-panel order and deferred omission.
Accordion additionally needs exact workspace capacity, no-handler ignored workspace,
disjoint-buffer preconditions and invalid admission atomicity. Verify reduced-motion
and exit-panel rendering do not add notifications or change callback counts.
Retain the existing Disclosure suite and strict C11/C++ header checks; native tests
use private display and browser fixtures headless once implementation is authorized.

C facades would not expose Python/JS/TS/Go/Rust typed disclosure events. Their queued
notifications need a separate schema/wrapper slice with full stable IDs, typed
expanded state, bounded ownership and exception containment; queued notifications
cannot reject past writes. See [binding audit](COMPONENT_EVENTS_BINDINGS.md).
This checkpoint ran no builds and changes no production source, manifests or ledgers.


## Revised preferred slice: per-transition reducer hooks

This amendment supersedes the aggregate/workspace recommendation above. Actual
write sites are narrow and available in `src/base/cui_disclosure.c`; neither
family needs caller snapshots, allocation, a command queue, nor a new dispatch
architecture. Keep the typed change/handler/result declarations above, but define
`change_count` as the number of **actual applied bool-transition callbacks**, with
a saturating `count_overflow` flag added to the result. `changed` remains the exact
legacy bool. The same entrypoint names are proposed, with **no workspace argument**:

```c
typedef struct {
    bool changed;
    size_t change_count;
    bool count_overflow;
} cui_disclosure_event_result;
cui_disclosure_event_result cui_base_collapsible_events(
    cui_context *, cui_id, cui_str label, bool *open, bool disabled,
    cui_disclosure_handler);
cui_disclosure_event_result cui_collapsible_events(
    cui_context *, cui_id, const cui_theme *, cui_str label, bool *open,
    cui_collapsible_options, cui_disclosure_panel_fn, void *panel_user,
    cui_disclosure_handler);
cui_disclosure_event_result cui_base_accordion_events(
    cui_context *, cui_id target, const cui_disclosure_item *, size_t count,
    bool *open, cui_accordion_options, cui_disclosure_handler);
cui_disclosure_event_result cui_accordion_events(
    cui_context *, cui_id container, const cui_theme *, const cui_disclosure_item *, size_t count,
    bool *open, cui_accordion_options, cui_disclosure_panel_fn, void *panel_user,
    cui_disclosure_handler);
```

### Exact assignment and notification sequence

Refactor each existing Base reducer into a private common implementation with an
optional dispatcher/result sink. Legacy functions pass NULL and preserve the old
statements/order, admission, semantics, routing, bool result and callback-free
behavior. Styled private common implementations choose the new Base entry at the
existing seam; do not wrap the old aggregate bool or reduce twice.

Collapsible's existing matching-activation loop executes `*open = !*open`, then
sets its local changed=true. Capture previous, perform that write and old bookkeeping,
then notify immediately, before the next input and before final semantic updates.
Two accepted toggles therefore emit true→false and false→true (or the inverse),
even if final state equals entry state. Missing handler never changes reduction.

Accordion processes inputs in existing event-loop order. When the current cursor
is eligible and `!open[cursor] || collapsible`:

1. Compute `next_open = !open[cursor]` as today.
2. If next_open and !multiple, visit j=0..count-1 exactly as today. For each
   `open[j] = false`, capture its preceding bool, do the write, and notify only if
   it was true. Already-false assignments stay silent. This includes disabled
   siblings, because the existing single-open policy clears them too.
3. Perform the existing `open[cursor] = next_open`, set changed=true as today,
   and notify for its actual bool transition. If closing a collapsible open item,
   no sibling reset occurs. With multiple=true, only this final assignment occurs.
4. Continue the existing keyboard focus logic and subsequent events, then execute
   the original final group semantic update loop without moving it earlier.

On opening, the selected cursor was false, so its reset-to-false assignment is
silent and its later false→true transition emits once. Opening another item can
emit several true→false siblings in model order, followed by the opened item's
false→true. There is no duplicate observer for an assignment that leaves a bool
equal. Do not suppress a real sibling close merely because that sibling is disabled.
Do not normalize arbitrary idle multi-open input or create events for animation.

### Metadata and timing honesty

At the accepted activation branch, before the **first** affected write/callback,
copy `cui_event_activation_snapshot` when available and initialize shared metadata
with `cui_component_event_from_activation`. Both reducers already call
`cui_interact` to register Button semantics before their activation loop, so genuine
accepted activation evidence must not deliberately be discarded. Override only
`metadata.target` with the explicit compound ID; retain the original snapshot's
`activation.target` unchanged. Payload item/item_index identifies the affected bool
slot. In particular, keyboard cursor redirection and automatic sibling closure do
not turn the originating activation into an activation of that affected item.

If an accepted snapshot is unavailable, freeze only the actual local frame/index
and known event-type source: pointer-up POINTER, key-down/up KEYBOARD,
accessibility-action ACCESSIBILITY, otherwise UNKNOWN. Do not fabricate activation
fields or infer a physical device/timestamp. Every transition caused by the same
activation shares this frozen original evidence, including automatic sibling
closes. An input index is causal identity, not an activation of the affected sibling.

Never register semantics early to manufacture evidence. Observe only a nonzero
active frame and ordinary painted, globally enabled submission. Core's inert
activation/target suppression remains authoritative. If metadata cannot be proven,
preserve legacy writes but emit no fabricated callback. Capture evidence once before
the closure loop; observers must not mutate active dispatch/input state. Existing
semantics are finalized at their original later points, not before notifications.

Callbacks are synchronous **immediately after each applied bool write**, not after
whole-event/group semantic finalization. They can read an intermediate group:
while opening in single mode, some siblings may have closed while later ones are
still open; immediately before the final open there may be no open panel. Existing
invalid multi-open input remains visible until its existing reset loop finishes.
No exactly-one-open invariant is promised inside the callback. `open` has the newly
written value, but semantic expanded flags, motion and Styled panels still reflect
their original later update points. Do not read them as already-finalized state.
Do not reposition semantics, render callbacks or spring stepping to conceal this.

Observer callbacks must not mutate, rebind, free or destroy the active expanded
array, items, context, states, style/motion arrays or callback resources, consume
input, or reenter UI. This includes owner writes to a sibling bool: these are
post-mutation notifications, not owner-rejectable requests. Unrelated application
flags may change, with omission/removal/reorder/teardown deferred until outer return
plus frame/render consumption. No new runtime mutation guard is implied. Legacy
render/header/panel callback order and ownership remain unchanged; new observer
rules do not authorize reordering their arbitrary child UI. The observer is not a
render slot and cannot submit CUI. Retained measurement still survives end_frame.

### Alternatives, risks and proof obligations

Immediate hooks are preferred because they preserve every actual transition with
constant auxiliary memory and no externally owned snapshot workspace. Their cost
is explicit intermediate-state/stale-semantics visibility and more callbacks than
an aggregate result suggests. Consumers needing only final state should coalesce
copied payloads in unrelated application storage and apply effects after submission;
that does not change native observer timing.

Dispatching all closures only after the opening write would preserve a stronger
within-event model invariant, but storing arbitrary changed siblings/previous bools
requires bounded caller/frame storage or a new count cap; the existing model has no
such bound. Reconstructing the old true set from the now-false array loses data.
A two-pass notification scan cannot recover it without storage. Do not introduce
hidden allocation, truncate callbacks or reorder writes for convenience. The initial
aggregate-workspace option is valid only as a separately chosen API and loses
repeated/net-zero transition information; it is not the preferred implementation.

New tests must assert exact ordered transition tuples, not only final state:
Collapsible double toggle gives two callbacks with unchanged final state; Accordion
single-open closes prior true slots in index order then opens cursor, including a
disabled-open sibling; multiple=true never closes siblings; collapsible=false open
activation produces none. Verify every callback's current slot value and explicitly
sample allowed intermediate arrays/stale semantics. Confirm metadata is frozen once
per cause, source/index and original copied activation same for sibling closes,
HAS_ACTIVATION when genuine snapshots exist, fallback only when unavailable, and repeated
same-frame inputs each retain their own index. Missing observers keep old results
and writes, counts are actual invocations, unused user ignored, saturation is safe.

Pair old/new contexts for state/focus/semantics/draw/motion/render-callback equivalence
after reduction; prove no early semantics, no double input, and unchanged custom
header child routing. Cover pointer/key/accessibility where genuinely supported,
consumed/inert/layout/disabled suppression without altering old maintenance,
copy-after-frame/destruction and flags-driven deferred omission. Run strict C11/C++
headers plus existing Disclosure tests via the private wrapper after approval.
This remains a signed design amendment only; no production implementation or tests
were added or run, and no foreign binding/lifecycle/parity claim follows.
