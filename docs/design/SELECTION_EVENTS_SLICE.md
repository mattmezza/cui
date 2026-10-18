# Single-selection event facade checkpoint

Status: proposal only; no new API below ships. Audited at `b61a14d0` against the [contract](COMPONENT_EVENTS_CONTRACT.md), [coverage](COMPONENT_EVENTS_COVERAGE.md), [bindings](COMPONENT_EVENTS_BINDINGS.md), and implemented [metadata](../COMPONENT_EVENTS.md). Scope is RadioGroup, Select and single-selection Combobox, without changing their reducers, ownership, popup timing or legacy results. No family parity promotion follows from this proposal.

## Source findings

| Family | Ownership and dispatch actually implemented | Result/multiplicity evidence |
| --- | --- | --- |
| RadioGroup Base/Styled | Caller owns `cui_id *selected` and focus state. Without `options.on_change`, reducer writes selection. With it, synchronous callback requests an item; owner accepts by writing `*selected`, or rejects. Later input reads the current owner value, not an immutable entry snapshot. Focus moves even on rejection; semantics/paint use the value after callbacks. | `valid/requested/changed/tab_stop` aggregate one call. Already-selected items do not request. Each eligible event can request; navigation advances from current focus. [header](../../include/cui/cui_radio_group.h), [Base event loop](../../src/base/cui_radio_group.c), [Styled forwarding/paint](../../src/components/cui_radio_group.c), [Base tests](../../tests/unit/test_radio_group_base.c), [Styled tests](../../tests/unit/test_radio_group.c). |
| Select | Caller owns selected ID and popup/list state; direct mutation. Public Select has render slots but no value callback. Internal list callbacks exist on `cui_list_options`, which Select constructs privately without activation callbacks. | List returns activation including same-item confirmation; Select closes popup on activation but returns `old != *selected` after the entire list pass. Several list activations can occur before that close; final bool cannot reconstruct transitions or same-item confirmations. [API](../../include/cui/cui_choice.h), [list reducer](../../src/base/cui_choice.c), [popup renderer](../../src/components/cui_choice.c), `test_lists_and_popups`, `test_choice_scrolling` and related cases in [choice tests](../../tests/unit/test_choice.c). |
| Combobox | Caller owns selected ID, query editor, filtered scratch and popup. Query changes independently of committed selection. Base processes query input, computes matching items, then handles Enter; Styled additionally runs a list pass for popup option activation. No public selection callback. | Result contains `changed/query_changed/scrolled/matching_count/error`, without per-transition history. Single Base Enter commit breaks its commit loop. Styled list can process multiple activations. Filtering leaves committed IDs intact. `test_combobox` and `test_combobox_text_enter_order` in [choice tests](../../tests/unit/test_choice.c) prove query/Enter order and popup state, including Enter followed by text selecting the previous active item. [Base](../../src/base/cui_choice.c), [Styled](../../src/components/cui_choice.c). |

RadioGroup consumes/cancels accepted item activation before its old callback. Its arrow/Home/End path consumes key-down directly; these are selection navigation, not Core activation. List invokes `on_activate` then `on_confirm` after assigning selection. Navigation/typeahead changes active focus without selection. Combobox Enter is a direct key path, not an activation snapshot. None of these facts permits synthesizing pointer evidence from current pointer position.

Existing tests prove legacy behavior, not the proposed facade. They do not yet prove typed evidence copying, callback mutation/consumption, saturated per-event counts, all passive modes, or cross-phase notification ordering. Exact named tests/source above establish those boundaries rather than a claim that bool results uniformly mean CHANGE.

## Recommended smallest compatible slice

Add **notification-only** single-selection observers. Keep RadioGroup's old request callback intact, and observe only actual transitions after it returns. Preserve direct mutation for Select/Combobox. A missing observer continues normal behavior; it is not read-only. This differs deliberately from the by-value controlled Check API. Uniform owner-rejectable Select/Combobox requests require a later reviewed controlled entry point; wrapping a local `selected` copy would break same-call focus, popup, semantics and query contracts.

Proposed new Base-dependent header `cui_selection_events.h` contains only metadata and pointer-free payload/result types; existing family headers declare forwarding functions. No Components dependency in the common header:

```c
typedef enum {
    CUI_SELECTION_REASON_ACTIVATION,
    CUI_SELECTION_REASON_NAVIGATION,
    CUI_SELECTION_REASON_CONFIRM
} cui_selection_reason;
typedef struct {
    cui_component_event_metadata event; /* CHANGE only */
    cui_id previous, value;              /* actual owner values; zero means none */
    cui_id item;                         /* attempted item, distinct from accepted value */
    cui_selection_reason reason;
} cui_selection_change;
typedef void (*cui_selection_change_fn)(void *, const cui_selection_change *);
typedef struct {
    cui_selection_change_fn on_change;
    void *user;
} cui_selection_handler;
typedef struct {
    bool valid, submitted, count_overflow;
    size_t change_count;
} cui_selection_event_result;
typedef struct {
    cui_selection_event_result events;
    cui_radio_group_result legacy;
} cui_radio_group_event_result;
typedef struct {
    cui_selection_event_result events;
    bool legacy_changed;
} cui_select_event_result;
typedef struct {
    cui_selection_event_result events;
    cui_combobox_result legacy;
} cui_combobox_event_result;
```

`change_count` counts actual observer callback invocations, saturating at `SIZE_MAX`. Missing observer preserves ordinary mutation and reports zero. `valid` means preflight succeeded, not that later rendering cannot fail. `submitted` follows existing active-submission policy (disabled can submit, inert/layout-only cannot dispatch). Preserve legacy aggregate results exactly, including surprising differences from transition counts. Payloads omit labels/query strings, indices and borrowed descriptors: IDs remain stable under reorder/filtering, and query text belongs to the edit-event API. An invalid/removed selection stays caller-owned rather than being normalized silently.

Exact proposed entry points (arguments intentionally match legacy order, then handler):

```c
cui_radio_group_event_result cui_base_radio_group_events(
    cui_context *, cui_id, const cui_radio_group_item *, size_t, cui_id *,
    cui_radio_group_state *, cui_radio_group_options, cui_selection_handler);
cui_radio_group_event_result cui_radio_group_events(
    cui_context *, cui_id, const cui_theme *, const cui_radio_group_item *, size_t,
    cui_id *, cui_radio_group_state *, cui_radio_group_options, cui_selection_handler);
cui_select_event_result cui_select_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, cui_str placeholder,
    const cui_choice_item *, size_t, cui_id *, cui_choice_popup *,
    cui_choice_popup_options, cui_selection_handler);
cui_combobox_event_result cui_base_combobox_events(
    cui_context *, cui_id, cui_str name, cui_edit *, const cui_choice_item *, size_t,
    cui_choice_item *, size_t, cui_id *, cui_choice_popup *, bool disabled,
    cui_selection_handler);
cui_combobox_event_result cui_combobox_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
    const cui_choice_item *, size_t, cui_choice_item *, size_t, cui_id *,
    cui_choice_popup *, cui_choice_popup_options, cui_selection_handler);
```

No new Base Select is invented: current Base exposes trigger and list primitives, not a full Select. Implementation needs private observer plumbing into existing reducers, with legacy calls supplying no observer; a wrapper comparing entry/exit values is insufficient.

## Timing, evidence and architectural decisions

For each eligible transition, freeze real input facts before calling mutable legacy callbacks; capture `previous` immediately before the existing write/request, run that existing operation, capture `value` immediately afterward, then synchronously notify if different. RadioGroup order is old request callback → new CHANGE observer → remaining events → semantics/paint. Select list order is assignment → existing list activation/confirmation sequence → observer → remaining list events → existing popup close/paint. Combobox Enter order is assignment → existing internal commit callback (where present) → observer → existing close decision. Do not enable observers for multi-select through shared helpers accidentally.

Observer payload is an immutable entry-to-observer snapshot. If an observer changes owner selection itself, that write is not another recursively synthesized notification; later events and painting see it according to the existing pointer-owned model. Owner acceptance is not inferred from equality with `item`: a RadioGroup request callback can assign another value, and CHANGE must report that actual value. Rejection with no transition emits no CHANGE. Same-item confirmations and query-only edits emit none. Programmatic assignments outside submission emit none.

**Approved decision 1: compound target identity.** `event.target` is the public group/trigger/input ID. `item` identifies the attempted option. A copied `activation.target` remains the actual activated child ID; do not rewrite it to impersonate a trigger. Shared helper `cui_component_event_from_activation` initially copies activation target into metadata target, so compound forwarding assigns the public compound ID to metadata after copying, retaining original activation. This approved convention adds no Core routing mechanism.

**Approved decision 2: preserve reducer order, not promise global input-index order.** Styled Combobox runs its Base query/Enter reducer before the popup list reducer. An Enter/text/pointer mixture can therefore yield callbacks in reducer-phase order rather than monotonically increasing event indices. Preserve that existing order and expose truthful indices; do not sort/defer callbacks or replay inputs to manufacture global ordering. Select likewise closes only after its list pass. Any requirement to close after the first activation or unify Combo event phases is a separate behavioral change.

For genuine accepted Core activation, retain `cui_activation_snapshot` copied before consumption/callbacks. Navigation and Combo Enter use `HAS_FRAME|HAS_INDEX`, source KEYBOARD, and **no HAS_ACTIVATION**; direct accessibility activation gets only evidence actually supplied by Core. Navigation reason is separate from physical source; touch is retained in real pointer evidence, not a new inferred source. No clock/device fields are invented. Missing evidence is explicit; unknown must not be labeled programmatic merely because a snapshot is absent.

The callback, user and payload are borrowed synchronously. Pointer-free copied payload survives context/frame lifetime; caller item arrays, edit/scratch/state, theme/render resources and users must remain valid through active submission/render completion as their existing APIs require. Copy handler/options at entry. Descriptor mutation affects later calls; freeing active arrays/users/state, context destruction, recursive same-frame submission and arbitrary teardown during dispatch remain unsupported. Consume-later-input effects remain observable; there is no prequeued callback batch or fake destruction guard.

## Implementation ownership and meaningful proof

Suggested implementation scope, pending approval: new `include/cui/cui_selection_events.h`; narrow declarations in `include/cui/cui_radio_group.h` and `include/cui/cui_choice.h`; private reducer/forwarding changes in `src/base/cui_radio_group.c`, `src/components/cui_radio_group.c`, `src/base/cui_choice.c`, `src/components/cui_choice.c`; new `tests/unit/test_selection_events.c`; human/LLM guide `docs/SELECTION_EVENTS.md`. Root owns module/CMake/generator/SDK/bindings integration. No Core/backend, manifest or existing option/result layout changes.

Tests must exercise actual reducers, not fabricated snapshots:

- RadioGroup accepted/rejected/redirected old requests, legacy-before-observer ordering/users, selected-item suppression, two arrows advancing focus and per-transition values; old result unchanged. Missing observer retains legacy writes.
- Select open→option pointer/touch/key/accessibility selection, multiple queued activations with final value returning to entry (two CHANGE notifications despite false final bool), same-item activation closing without CHANGE, scrolling invalidating old pointer targets, outside dismissal without selection.
- Combo query-only edits and filtered-out committed IDs, both existing text/Enter order cases, Enter selection and popup pointer selection, same-item confirmation, phase-order mixed input records, matching/scratch errors, no duplicate events from Base+Styled forwarding.
- Freeze activation before old callback consumes accessibility input; keyboard navigation/Enter retain index/source without false activation. Copy payload after frame/context destruction; exact child versus control target convention. Observer consumes later input, updates owner, schedules deferred removal; no reentrant synthesized notifications.
- Disabled group/item, empty/all-disabled lists, inert/layout-only/missing invocation, invalid arguments, missing handler, count saturation; legacy suites remain unchanged. Strict C11/C++ public-header compilation and registered headless CTest through `python scripts/test_local.py --build-dir build` only.

Deferred explicitly: controlled Select/Combo request/rejection, RadioGroup typed request replacement, query-edit events, same-item ACTIVATE/COMMIT, open/close lifecycle, menu/list/multi-select/tabs/toggle-group facades, async/deferred listeners, foreign-language synchronous requests, bridge typed selection allowlist, label/text copying and uniform 78-family coverage. Existing bridge queued notifications keep their timing and current allowlist.

Approved implementation sequence: first RadioGroup Base/Styled only, with minimal shared selection types. Select/Combobox signatures above remain proposals for a separate later slice. No choice reducer edits are authorized by the RadioGroup implementation.
