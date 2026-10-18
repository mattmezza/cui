# Tree applied events: proposed slices

Status: design checkpoint only, audited at `9f8a8bc5`. Nothing below introduces an
implemented public API or promotes the 78-family ledger. Preserve the current
[component contract](COMPONENT_EVENTS_CONTRACT.md), old functions, option/state
layouts, reducer order, results and bridge delivery timing.

## Existing source contract

| Surface | Actual ownership and evidence |
| --- | --- |
| `include/cui/cui_tree.h`, `src/base/cui_tree.c:cui_tree_rebuild` | Caller owns preorder nodes, label bytes and expanded bool array; model borrows them until next successful rebuild. Keys are unique/nonzero; depth starts at zero, rises by at most one, maximum 63. Two preallocated banks make rebuild validation atomic. Capacity 1..100000; no allocation after create. |
| Model queries / `cui_tree_set_expanded` | Model retains indexes, parent/subtree/visible mappings. Setting a branch's different bool writes caller storage then rebuilds mappings; same value, missing key or leaf returns false. This is not a rejectable owner request. External direct array edits require rebuild. There is no incremental insert/remove/move/rename API or callback. |
| `cui_tree_state` | Caller owns cursor, bounded ASCII search buffer/time and scroll. Reducer repairs cursor and search state before events, then clamps scroll afterward. Those maintenance changes are not selection/expansion/activation events. |
| `cui_tree_reduce` | Processes unconsumed inputs in order, skips global/item disabled input, mutates selected key and expanded array. No existing user callback. `selection_changed` compares only entry/final selection; `expansion_changed` ORs successful expansion operations; `activated` retains only the last row activation key. Repeated/net-zero input is therefore lost from these aggregate results. |
| `cui_base_tree`, `cui_tree` | Reduce once, then submit visible semantic rows/toggles. Styled additionally handles scrollbar, layout, chevrons and text; no content/render slots. Base caller supplies visible row/toggle geometry. No observer should add a second reduction or move submit earlier. |

`tests/unit/test_tree.c` already covers `models`, `random_models` (1000 rebuild
trials), `events` (all themes, repeated navigation, expansion, typeahead, manual
selection), `disabled_hierarchy`, `virtualization` (10000 nodes),
`tree_toolbar_artwork` (consumed Left/Right and actual chevron bitmap changes), and
`tree_scrollbar_skin`. The same source has a Base-only path. These tests establish
existing behavior, not a proposed event facade.

## Preferred additive API

Propose new `include/cui/cui_tree_events.h`, including Tree and shared metadata:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_id previous, value;
    size_t node_index; /* affected value's current model index */
} cui_tree_selection_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id key;
    size_t node_index;
    bool previous, expanded;
} cui_tree_expansion_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id key;
    size_t node_index;
} cui_tree_activation;
typedef struct {
    void (*on_selection_change)(void *, const cui_tree_selection_change *);
    void (*on_expansion_change)(void *, const cui_tree_expansion_change *);
    void (*on_activate)(void *, const cui_tree_activation *);
    void *user;
} cui_tree_handler;
typedef struct {
    cui_tree_result result; /* exact legacy aggregate result, unchanged */
    size_t selection_count, expansion_count, activation_count;
    bool count_overflow; /* actual calls, independently saturating at SIZE_MAX */
} cui_tree_event_result;
```

Add `cui_tree_reduce_events`, `cui_base_tree_events` and `cui_tree_events`, with
existing corresponding arguments plus trailing handler, returning the new result.
Use narrow common reducers/Styled entry, with old entry points passing no observer.
Missing observers never make the widget readonly; unused user is ignored. Require
nonnull context/model/state/selected and nonzero compound ID before new admission;
return `.result.error=CUI_TREE_INVALID` atomically on missing required arguments.
Existing model/key/label lifetime preconditions remain. No workspace, allocation,
event cap, change to old layouts or generic dispatch architecture.

Separate callbacks distinguish typed domains without overloading shared CHANGE:
selection and expansion use CHANGE; row invocation uses ACTIVATE. Index is a model
slot at delivery, not an event index or persistent identity. Keys are stable across
rebuild/reorder when caller preserves them. Do not borrow labels/model pointers in
payloads, invent previous-value validity, or equate cursor movement with selection.

## Exact seams and ordering

1. **Selection slice first.** Hook both actual `*selected` assignments in
   `cui_tree_reduce`: row activation and automatic navigation/typeahead. Capture
   before, retain the write, then notify only when before differs from after.
   Row notification occurs before the existing cursor/result.activated assignments;
   navigation notification occurs after cursor update but before existing focus.
   Do not silently strengthen callback-visible state to operation-final state.
   `manual_selection` suppresses navigation selection writes, not row activation
   selection. Repeated A→B→A produces two callbacks even when old result says false.
2. **Expansion slice second.** At the toggle and keyboard Left/Right call sites,
   capture key/index/before and originating input evidence, perform the old
   `cui_tree_set_expanded` exactly once, then notify for an actual applied bool
   difference before existing cursor/focus/consume work. Successful rebuild has
   already refreshed visible mappings; do not notify inside the setter between
   bool write and bank swap. Normal valid borrowed model data makes rebuild
   succeed. If it fails after writing (setter is not rollback-atomic), compare the
   actual bool and notify that applied difference honestly; retain old false return
   and aggregate flag. Callback must not assume valid new mappings on failure.
   No observer on programmatic setter/rebuild or external edits in this slice.
3. **Activation slice third.** At the row-activation branch, after existing selected,
   cursor and `result.activated` assignments, notify every row activation including
   already-selected rows. Selection CHANGE (if any) precedes ACTIVATE. Toggle
   activation expands only; it does not invoke a row. Keyboard navigation, search,
   focus, scrolling, repair, expansion and animation never manufacture ACTIVATE.

Capture causal metadata before the first relevant write/callback for an event;
reuse that frozen copy for its later notifications. Do not retain bank pointers
across `cui_tree_set_expanded`: rebuild swaps banks. Hook values/key/index, not a
pointer into mutable model caches. No notification is delayed until whole-frame
finalization. Legacy final invalidation, scroll reveal/clamp, consume operations,
semantic submission and Styled scrollbar/paint remain at their original positions.
Thus callbacks can observe intermediate cursor/focus/scroll and old or absent
current-frame semantics; they cannot treat the whole tree as finalized.

## Genuine metadata, passive work and lifetimes

Metadata target is the compound ID; payload key identifies the affected node.
Try `cui_event_activation_snapshot` and shared conversion before writes whenever
accepted evidence exists. Preserve copied `activation.target` (row/toggle origin)
when overriding compound `event.target`, including cursor redirection. Never drop
available genuine evidence for convenience. However normal Tree reduce precedes
`cui_tree_submit`; Core accepted snapshots require matching current-frame semantic
registration (`src/core/cui.c:cui_event_accepted_activation`). Therefore snapshots
are not guaranteed at this seam. Do not call submit/interact early to obtain one.

When unavailable, attach only proven nonzero active frame, actual local input index
and known type-source: pointer-up POINTER, key-down/up KEYBOARD, accessibility-action
ACCESSIBILITY; TEXT UNKNOWN (text does not prove a keyboard device). No borrowed
raw text/event copy, fake activation, inferred device or invented timestamp. Left/
Right are genuine keyboard changes without accepted activation; typeahead selection
is genuine indexed input with UNKNOWN source. Capture before existing event consume.

Observe only active painted, globally enabled submission with nonzero frame;
Core inert target/activation suppression remains authoritative. Preserve old
maintenance and any legacy passive behavior while suppressing observation of it.
Disabled item input, consumed input and no-op writes are silent. No new focus,
scroll, hover, visibility, cursor-repair, programmatic expansion or lifecycle event.

Handler/user/payload borrowed during the operation; pointer-free payloads may be
copied by value. Observer must not edit/rebind/free nodes, labels, expanded array,
selected/state/model, rebuild/set expansion, consume/enqueue input, recursively
submit UI or destroy context/resources. Such mutations can invalidate iteration,
bank mappings or later semantics; this is notification, not owner rejection.
Unrelated app flags may change; defer structural edits/removal/omission/teardown
until outer return plus renderer consumption. Model survives omission until explicit
destroy; omitting submission does not rebuild/reset it or emit lifecycle events.
Asynchronous child loading/rename/move/context actions are app work with stable-key
and generation/lifetime policy, not supported Tree callbacks; design separately.

## Binding boundary and independent proof

`src/bridge/cui_bridge.c:bridge_render_tree` owns a separate 64-node model and arrays,
rebuilds from flat Item children each frame, calls legacy Styled Tree, then reports
aggregate selection change on the tree and net expansion changes on child records.
It does not forward `result.activated`. `bindings/python/test_model_components.py`,
`bindings/node/test/models.test.cjs`, `bindings/go/cui/native_test.go:TestTree` and
`bindings/rust/tests/builders.rs:tree_selection_and_expansion` cover existing model
wrappers. Python/JS/TS/Go/Rust builders do not expose this proposed typed immediate
C observer or its per-transition multiplicity. Generic queued change callbacks
remain post-mutation, with their existing exception/reentrancy/copy boundaries;
no synchronous request/rejection or new wrapper ABI follows from this proposal.

Each slice should add focused `tests/unit/test_tree_events.c` tests, alongside old
Tree and Base-only regressions, before independent signed integration:

- Pair old/new models/contexts for exact results, selection, expansion, cursor,
  search, focus, scroll, consumed bits, visible mappings, semantics and draw output;
  compare callbacks absent/present without double editing or changed geometry.
- Assert ordered multi-input selection/net-zero tuples, no-op silence, manual
  navigation versus row selection, text-prefix repeat/timeout and UNKNOWN source.
- Assert toggle open/close, Left/Right branch/parent navigation, repeated/net-zero
  expansion, immediate consistent post-rebuild visibility and stale semantic timing.
- Assert selection-before-activation, repeated already-selected invocation, last
  legacy activated versus actual callback count, row/toggle distinction and a11y
  only where real existing routing supports it. Copy snapshot evidence when caller
  has already legitimately registered semantics; prove fallback in normal reducer
  order and no fabricated activation on navigation/typeahead.
- Exercise disabled/inert/layout/consumed input, offscreen/clipped rows, hidden or
  removed cursor repair, stable-key rebuild/reorder, unrelated flags/deferred
  omission, copy after frame/model teardown, scrollbar-only silence and 10000-node
  viewport behavior. New observers must not weaken existing model validation tests.
- Strict C11/C++ header orders; private-Xvfb wrapper for focused native/portable
  tests. Browser/FFI proof is a later explicit slice, not inferred from C tests.

Proposed ownership on approval: new header/test/guide, narrow existing Base and
Styled Tree common seams, existing header discoverability comment and this proposal.
Root retains build/source registration, generated exports/SDKs and binding work.
Aggregate-only wrappers are simpler but lose exact transitions/invocations; there
are narrow real assignment seams here, so they are not the preferred alternative.
No production edits or builds were performed for this checkpoint.

## Selection implementation checkpoint

The first approved slice implements Base/reduce-only/Styled per-write selection
notifications through a shared reducer seam. Expansion and activation observers
remain pending independent commits. Legacy aggregate results and public layouts
are unchanged; no model setter or binding callback is added by this slice.

## Expansion implementation checkpoint

The second approved slice adds per-applied-bool expansion notifications after the
existing setter/rebuild attempt, preserving all aggregate flags and later consume,
focus and semantic work. Programmatic setter/rebuild calls remain unobserved.
Activation observers remain pending their independent commit.

## Activation implementation checkpoint

The third approved slice adds every existing row invocation after its original
selected/cursor/result assignments, with selection CHANGE first when applicable.
All three hooks reuse evidence frozen before writes; only the activation copy's
kind changes. Legacy last-activated result and mutation/consume/paint order remain.
Current Core TREEITEM accessibility ACTIVATE is unsupported/rejected; Button toggle
activation is separately supported. This slice does not alter that routing contract.
