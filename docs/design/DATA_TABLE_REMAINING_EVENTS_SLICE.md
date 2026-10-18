# Remaining DataTable events checkpoint

Audited at `4aa3c3d0`. The independently approved activation/context slice is now
implemented; mode requests below remain a design checkpoint awaiting their own
production slice. This supplements the
[original checkpoint](DATA_TABLE_EVENTS_SLICE.md), follows the
[shared contract](COMPONENT_EVENTS_CONTRACT.md), and preserves the current
[implemented facades](../TABLE_EVENTS.md). The activation/context implementation
adds only the reviewed native facade fields/counters and narrow reducer seams. No new dispatch, Core behavior, bridge
schema, generated files or 78-family manifest entries change.

## What the remaining results mean

The [public result/options](../../include/cui/cui_table.h#L109) combine input
facts with reduction-completion facts. They are not one callback transcript.
Model rows/columns and state remain caller-owned; options, including selection
mode, are passed by value. Changing sort state does not reorder application rows.
Existing scalar/set/column observers already cover applied value transitions.

| Domain | Actual seam and information | Recommended treatment |
| --- | --- | --- |
| Cell activation | [Cell branch](../../src/base/cui_table.c#L1032): accepted activation attempts selection, sets reveal, then writes latest activated row/column. Same-value or failed selection still activates. | Per-accepted-action ACTIVATE, independent of CHANGE. |
| Row-header activation | [Row-header branch](../../src/base/cui_table.c#L725): attempts effective ROW selection, writes activated row, then possibly requests ROW mode. | Per-accepted-action ACTIVATE with explicit ROW_HEADER area. |
| Context request | [Row header](../../src/base/cui_table.c#L717) and [header/cell](../../src/base/cui_table.c#L954): secondary POINTER_DOWN writes target IDs. Does not apply selection or open a popup. | Per-branch REQUEST identifying area and exact keys. |
| Selection-mode request | [Row header](../../src/base/cui_table.c#L729): activation or eligible navigation asks caller to use ROW options, after selection was already attempted in ROW mode. | Per-branch REQUEST; notification of intent, no rejection/rollback hook. |
| Visible windows | [Completion](../../src/base/cui_table.c#L1168): current full views plus flags comparing prior begin/end indices, initialized on first submission. | Keep the existing aggregate result; not a per-input scroll event. |
| Load more | [Completion](../../src/base/cui_table.c#L1176): near-end pending edge, suppressed by disabled/loading and reset by leaving threshold, row-count change or option off. | Keep the existing edge result and application-owned asynchronous work. |

Important compatibility details:

- `activated_row/column` retain last writes, not counts. Row-header activation
  writes **only row**: after an earlier cell action in the same call the final
  aggregate can contain that cell's column with the later row-header row. Do not
  reconstruct a typed action from the returned pair, clear the old column, or
  reinterpret this legacy aggregate while adding observers. Keyboard actions use
  the reducer's effective cursor, which need not equal the original routed child's
  key after prior navigation; freeze those actual branch facts, not a target-ID
  reconstruction.
- Activation is not selection success. Nonselectable cells, selection-only
  disabled policies, unchanged selection and CUI_TABLE_CAPACITY can still reach
  the accepted-action branch. No inferred `applied`/`committed` bool is justified.
  Header sorting/selection already has typed observers; do not add a generic
  header-invoke callback merely alongside them in this slice.
- Context facts refer to the actual hit item, independently of controlled
  selection. Header requests have row=0; row-header requests have column=0.
  Header/cell paths reject disabled columns and disabled rows as appropriate;
  row-header handling rejects disabled rows but works independently of disabled
  data columns. Global disabled/loading suppresses the input loop.
- Row-header mode requests are not options mutations. Activation requests ROW
  whenever row selection is enabled, even when options already say ROW or the
  selection operation failed. Navigation requests ROW when a candidate exists,
  manual selection is off and row selection is enabled. HOME/END can identify
  the current row and still request mode; do not deduplicate that intent into a
  value CHANGE. Manual navigation can move focus without a request. The flag
  being false makes `requested_selection_mode` non-actionable.
- Drag/cancel, child controls, routing and consumed-event treatment remain as
  implemented. The Table loop does not globally skip consumed events. Adding
  such a filter or treating observer input consumption as rejection would change
  behavior. A real header drop is not a cell action or context request.

## Reviewed input slices and exact payloads

Extend only the existing additive `cui_table_events.h` facades; preserve old public
state/options/result layouts and all entrypoints. Both scalar and set handlers
must gain the same implemented fields, before `user`, only as each slice lands.
The unreleased additive structs require rebuilding clients; use designated C
initializers. No retained registration or extra workspace is needed for these
pointer-free facts.

```c
typedef enum {
    CUI_TABLE_ACTION_CELL,
    CUI_TABLE_ACTION_ROW_HEADER
} cui_table_action_area;
typedef struct {
    cui_component_event_metadata event; /* ACTIVATE */
    cui_table_action_area area;
    cui_id row, column; /* ROW_HEADER always column=0. */
} cui_table_activation;
typedef void (*cui_table_activation_fn)(void *user, const cui_table_activation *event);

typedef enum {
    CUI_TABLE_CONTEXT_CELL,
    CUI_TABLE_CONTEXT_ROW_HEADER,
    CUI_TABLE_CONTEXT_COLUMN_HEADER
} cui_table_context_area;
typedef struct {
    cui_component_event_metadata event; /* REQUEST */
    cui_table_context_area area;
    cui_id row, column;
} cui_table_context_request;
typedef void (*cui_table_context_request_fn)(void *user,
    const cui_table_context_request *event);

typedef enum {
    CUI_TABLE_MODE_FROM_ACTIVATION,
    CUI_TABLE_MODE_FROM_NAVIGATION
} cui_table_mode_request_reason;
typedef struct {
    cui_component_event_metadata event; /* REQUEST, not an applied mode CHANGE. */
    cui_table_selection_mode current, requested; /* Current options snapshot. */
    cui_id row; /* Effective row at this operation, not a retained row pointer. */
    cui_table_mode_request_reason reason;
} cui_table_mode_request;
typedef void (*cui_table_mode_request_fn)(void *user, const cui_table_mode_request *event);
```

1. **Implemented activation and context slice:** add optional `on_activate` and
   `on_context_request`, and shared result counters `activation_count` and
   `context_request_count`. These recover repeated accepted attempts and repeated
   requests hidden by the old last-value results. Count actual synchronous calls,
   saturating with the existing `count_overflow`. Context does not synthesize
   activation. No coordinates, menu ownership, acceptance return or popup lifecycle
   are promised; caller can identify the item with keys/area and existing ID helpers.
2. **Pending independent mode-request slice:** independently add `on_mode_request` and
   `mode_request_count`. This is deliberately separate from selection CHANGE:
   the application decides next submission's options, but ignoring the request
   does not undo the current effective ROW selection attempt. No generalized
   controlled-selection or options-rejection protocol is introduced.

A missing callback ignores unused userdata and retains every old write. Existing
scalar `on_selection + multiple_selection` rejection and set-workspace preflight
remain unchanged, including atomic failure before callbacks. New input observers
must also work with sort/column-only set handlers and ignored workspace when the
set-selection callback is NULL. Do not expose the later slice's fields early.

## Ordering, evidence and lifetime

Freeze exact IDs, effective area/mode/reason and metadata at the actual branch
before the first potentially user-calling selection operation. Do not reconstruct
IDs from the final result or look up a moved descriptor after another observer.
Preserve source order and existing observer timing:

- Cell: selection attempt/optional CHANGE, old activated-pair writes, ACTIVATE.
- Row header: selection attempt/optional CHANGE, old activated-row write,
  ACTIVATE, old mode-request writes, REQUEST if the legacy flag is true.
- Row-header navigation: update navigation cursor, selection attempt/optional
  CHANGE, old mode-request writes, REQUEST, then existing focus/reveal work.
  A mode callback must not assume focus has already moved.
- Context: old context writes, REQUEST at that branch, then existing cursor/
  header/navigation handling. It must not assume the context item is selected.

Metadata target is the table root. Preserve the actual activation snapshot where
available; frame/index/source are genuine operation evidence. Secondary press
has POINTER_DOWN provenance but normally no activation: extend the private source
classification for this actual input, never fabricate HAS_ACTIVATION. Navigation
requests use their real key event, not reconstructed activation. Unknown evidence
remains absent. All these callbacks precede final semantics and Styled rendering;
none promises refreshed bounds or final viewport state.

Callbacks and userdata borrow through the current submission. Payloads can be
copied by value; they contain stable native IDs and no row/text pointers. Existing
borrowed row/column/label/group/render storage and renderer-consumption lifetimes
still apply. No mutation/rebinding/sorting of active owner/model/workspace,
recursive submission, destruction or foreign unwinding is supported. Record
application intent and defer changes/teardown until the outer call and renderer
consumption finish. No retained observer registration or automatic binding fanout.

## Justified completion-result exceptions

The original checkpoint tentatively suggested aggregate completion observers.
This revision recommends retaining result-driven consumption instead: no extra
transition facts were found to justify those callbacks yet.

Do **not** add `on_scroll`, `on_visible_rows`, `on_visible_columns` and
`on_load_more` merely to replay existing aggregate fields. The existing result
already provides the complete supported completion observation once per call:

- Final `rows/columns` follow wheel processing, reveal, clamp and Base scrollbar
  reduction. Their windows use overscan=0; Styled rendering can visit a larger
  overscanned window. Header/fixed regions have separate layout rules.
- Visible flags compare only entry bookkeeping's begin/end indices, not pixel
  offset or total/width/height changes. First submission marks both flags, even
  without input. Earlier full views were never stored; only prior indices are
  available. A same-window pixel scroll is not a missing transition callback.
- `load_more` means a newly raised pending edge, including an enabled empty
  table. It does not mean data arrived, a fetch started/completed, or an accepted
  request. Supplying data/new row count can re-arm the edge; a pending bit is not
  a network cancellation or generation mechanism.
- Disabled/loading suppress user actions and the load edge, but normalization,
  initialization and bookkeeping still run. Inert/layout-only submission removes
  normal input interaction, not this aggregate computation: do not promise an
  unconditional no-result-change/no-load-edge rule for passive submissions.

Keep caller code reading `result.control` (or `set_result.events.control`) after
submission and scheduling application work there. This is an explicit aggregate
exception, not missing per-transition coverage. Do not add multiple callback side
effects alongside the same result use. If a concrete consumer later requires a
common completion hook, review **one** separate opt-in completion snapshot after
all reducer bookkeeping, before semantics/paint: include initialized/prior-index
presence, current full views, changed flags and load edge. It would carry
UNKNOWN/HAS_FRAME only when an active frame is proven, no index/source/activation,
and no claimed old full view or per-wheel attribution. That future hook is not
part of the recommended production slices here.

[Styled](../../src/components/cui_table.c#L394) resolves theme scrollbar policy
before reducing once, then paints. Its late
[scrollbar call](../../src/components/cui_scroll.c#L59) accepts const state and
paints; do not move callbacks into paint or rerun reduction to obtain a viewport.
Base wrappers submit semantics for OK/CAPACITY; invalid preflight/arguments do not
produce input callbacks. Earlier successful observations survive later capacity
errors, as in the existing facades.

## Required evidence and explicit omissions

Keep [legacy Table](../../tests/unit/test_table.c),
[selection contracts](../../tests/unit/test_table_selection_contracts.c) and
[typed transitions](../../tests/unit/test_table_events.c) unchanged in meaning.
Add actual queued pointer/key tests through reducer/Base/Styled scalar and set
facades, with real Styled geometry priming where Base has no painted bounds:

- Repeated same-cell and same-row-header activation, several different items in
  one frame, selection-disabled/nonselectable activation and capacity rejection;
  exact copied keys above 2^53/near UINT64_MAX and mixed cell then row-header action
  whose legacy aggregate retains the earlier column. Verify counts and action area.
- Secondary presses on cell/row/column headers, repeated context requests with
  unchanged owner selection, disabled row/column distinctions and mixed action/
  request traces. No popup creation or simulated primary activation as a shortcut.
- Mode requests with options CELL and already ROW; activation versus navigation;
  manual navigation, row-selection-disabled, same-row HOME, repeated requests and
  capacity-after-progress. Verify selection CHANGE -> ACTIVATE -> mode REQUEST
  order where applicable and mode callback before later focus work.
- Missing callbacks/non-NULL userdata, passive/global disabled/loading, consumed
  input compatibility, callback copies and deferred teardown intent; paired old/new
  results, owner arrays/state, semantics, geometry and paint. No active owner edits
  inside test observers. Strict C11/C++ checks and private-wrapper CTest per slice.
- Preserve existing aggregate evidence: first windows, near-end pending/no replay,
  row-count re-arm, empty table and loading placeholders. Add targeted passive and
  same-window pixel-offset evidence before any later completion-hook proposal.

Current Core permits accessibility FOCUS for these focusable roles, but its
[ACTIVATE allowlist](../../src/core/cui.c#L1246) excludes ROWHEADER/COLUMNHEADER/
GRIDCELL unless explicit pointer_activatable evidence exists. Table submission
sets no such override. Test supported focus and rejected ACTIVATE truthfully;
no role remap, raw input fabrication or full accessibility parity claim.

The [bridge](../../src/bridge/cui_bridge.c#L3650) still calls legacy Styled Table,
queues aggregate selection/sort only, and returns early on Table errors. Native
activation/context/mode multiplicity, exact cui_id keys and completion facts are
not exposed by initial V1 or the additive value extension. Python/Node/Go/Rust
polling remains a separate post-step owned-copy contract; these C proposals add
no queued binding events, immediate handlers, request rejection or reordering.


## Activation/context implementation evidence

The narrow reducer seam freezes effective IDs and metadata before selection
observers, then delivers accepted actions at the old write points, including
unchanged/selection-disabled/nonselectable/capacity attempts. Context remains a
secondary-press request at its existing fields, with no activation or owner write.
Scalar/set facades share the new fields/counters; NULL set-selection permits
ignored workspace. No mode-request/completion callback is exposed.

Tests preserve the bounded 256-element/1024-command context fixture while keeping
4096-entry selection coverage. Actual queued keys/pointers cover repeated actions,
effective keyboard cursor IDs, mixed cell/row-header legacy column carry, metadata
before selection-observer consumption, all three context areas, disabled versus
nonselectable distinctions, passive/loading policies, optional observers,
callback-before-paint and paired legacy owner/result/semantics/geometry/paint.
Accessibility focus is exercised and ACTIVATE rejection checked at the public
queue API. Payload copies/deferred teardown intent retain existing lifetime rules.
