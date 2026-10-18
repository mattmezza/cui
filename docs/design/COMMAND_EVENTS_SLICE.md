# Command observer slice: review checkpoint

Status: proposal only; no new Command API is implemented by this document.
Audited integration `ca50785b`: `cui_command.h`, Base rebuild/select/move/confirm,
Styled inline palette and existing `test_command.c`. This refines the historical
Command row in COMPONENT_EVENTS_COVERAGE, without changing its baseline.

## Existing ownership and timing

The caller owns rows, query editor, state, entries, items and callbacks. Rebuild
borrows entry/item pointers until the next rebuild. It installs rows and selection,
then (for changed query) calls on_select if the resulting eligible path changed,
then on_query with borrowed raw UTF-8. Initial/unchanged rebuild does not call those
hooks. Select stores the row and pending-scroll policy before on_select. Move
calls Select. Confirm snapshots row, on_confirm and user before invoking item.action,
then invokes the captured on_confirm, even if the action changes selected state.
Styled calls rebuild before geometry validation, renders/reduces query, rebuilds
on aggregate input change, selects newly hovered rows, and selects then confirms
an activated row after its paint. Footer runs before the keyboard loop. Keyboard
arrows move, Enter confirms, Escape clears/rebuilds a nonempty searchable query;
otherwise it calls on_cancel only when installed. Existing disabled/loading,
consumption and focus behavior is operation-specific. A later false return may
follow successful earlier mutation/callbacks. These facts must remain unchanged.

## Compatible contract and implementation slices

Use a new header `cui_command_events.h` and additive observed entry points;
existing public structs and functions remain ABI/source compatible. Implement
private common reducer/render functions, with NULL observer on legacy paths.
Do not wrap all of Command in a final-state diff, replace old option.user, or
change the query to the per-input Input observer API: its current aggregate
query callback timing is intentional compatibility behavior.

1. Base observed rebuild/select/move/confirm with pointer-free typed selected
   path snapshots (section,row), optional previous path and shared metadata;
   query callback borrowing raw UTF-8; ACTIVATE confirmation snapshot. Each exact
   pointer-free callback seam freezes payload before legacy callback, invokes
   that callback with original userdata, then invokes the borrowed observer.
   Borrowed query observation has the explicit ordering exception below. Confirm
   preserves action -> captured legacy on_confirm -> new ACTIVATE ordering and
   snapshot identity. Counts summarize actual observer calls, even if old callbacks
   subsequently redirect state. Programmatic entry points use PROGRAMMATIC
   without invented frame/input metadata. Explicit clear-selection remains an
   existing silent helper unless a separate observed variant is designed.
2. Styled observed submission threads a private observation context through the
   same reducers. Query notifications remain aggregate per actual rebuild.
   Pointer hover selection is CHANGE, with only actual known pointer evidence;
   accepted row activation uses unique genuine snapshots when available, otherwise
   frame UNKNOWN (no invented index). Keyboard loop has actual frame/index/key
   evidence frozen before old callbacks; Enter is ACTIVATE, eligible Escape cancel
   is REQUEST to dismiss, not proof of dismissed hosting surface. Existing option
   on_cancel presence controls the legacy path; the new cancel handler can enable
   only the additive entry point's cancel notification, without implicitly consuming
   Escape, closing an overlay, or clearing query. Disabled suppresses new UI
   interaction observations; loading retains old policy. Initial/passive model
   rebuild callbacks require an explicit PROGRAMMATIC classification and must not
   be mislabeled user input. Inert/layout-only submissions must not introduce UI
   interaction callbacks. Custom content/matches/binding remain separate contracts.

Before production, the worker must refine exact type/function spellings, how
new-handler-only selection/query notifications map existing callback predicates,
and passive/inert behavior against actual source tests. Do not suppress existing
legacy callbacks merely to impose new policy. Root reviews the refinement.

## Ownership, alternatives and limits

Observer handler/user and payload views are borrowed for the outer operation.
Copy paths/metadata by value and query through bounded UTF-8 copy for deferred
work. No heap allocations, queue, propagation tree, deferred dispatch or generic
veto. Pointer-free CHANGE/ACTIVATE/REQUEST follow their documented legacy seam;
borrowed query CHANGE uses the explicit safe pre-hook exception below;
acceptance, consumption and platform defaults remain independent. Do not destroy
context/active resources or recursively submit UI during observers. Existing
legacy callback-driven selection changes remain observable at their old timing;
new observers may copy or set unrelated application flags and defer teardown.

Appending callbacks to public options was rejected to preserve old layouts.
End-of-frame replay loses ordering and mutations. Polling only the final selected
row loses hover/keyboard multiplicity and action snapshot identity. Emitting a
query event for each text edit would change legacy aggregate query timing.

No Python/JS/Go/Rust bridge equivalence is promised: native synchronous callbacks,
query-vs-selected-path values and action payloads are not current bridge V1 value
categories. SDK distributions include the C headers; owning language facades
remain explicitly omitted until their own protocol is designed.

## Verification and ownership

Own new public header, Base/Styled Command, dedicated new headless tests, new guide
and this checkpoint status only. Root owns CMake/WASI/source metadata, website,
usage inventory, generated SDK/distributions/skill. Use isolated worktree.

Compare legacy/new reducer state, callback order, mutation redirects, false-return
progress and rendering. Cover repeated confirms/moves in a frame, query+selection
ordering, action mutating selection before on_confirm, raw UTF-8 lifetime/copy,
handler/userdata routing, disabled/loading/inert/passive/omitted components,
pointer hover vs accepted activation, real keyboard and actual accessibility role
eligibility. Verify counts never replay or mutate. Run warnings, portable C/C++
headers, private-Xvfb native tests and applicable WASI/sanitizers. Never use desktop.

## Refined review contract (audited `91fab8fc`)

This section supersedes the provisional spellings/classification above. Production
is still unapproved. No new public layout, behavior or generated file is changed.

### Exact additive API

`cui_command_events.h` includes Command and shared component metadata. Proposed
public declarations (all callbacks are user-first):

```c
typedef struct {
    cui_component_event_metadata event; /* CHANGE */
    bool has_previous;
    cui_command_index previous, value;
} cui_command_selection_change;
typedef struct {
    cui_component_event_metadata event; /* CHANGE */
    cui_str value; /* Borrowed raw, untrimmed UTF-8; not an owned snapshot. */
} cui_command_query_change;
typedef struct {
    cui_component_event_metadata event; /* ACTIVATE */
    cui_command_index value; /* Confirmed snapshot, not later selected state. */
} cui_command_activation;
typedef struct {
    cui_component_event_metadata event; /* REQUEST to dismiss host */
} cui_command_cancel_request;
typedef void (*cui_command_selection_change_fn)(void *, const cui_command_selection_change *);
typedef void (*cui_command_query_change_fn)(void *, const cui_command_query_change *);
typedef void (*cui_command_activation_fn)(void *, const cui_command_activation *);
typedef void (*cui_command_cancel_request_fn)(void *, const cui_command_cancel_request *);
typedef struct {
    cui_command_selection_change_fn on_selection;
    cui_command_query_change_fn on_query;
    cui_command_activation_fn on_activate;
    cui_command_cancel_request_fn on_cancel;
    void *user;
} cui_command_event_handler;
typedef struct {
    bool control; /* Exact old bool return; false is not necessarily an error. */
    bool count_overflow;
    size_t selection_count, query_count, activation_count, cancel_request_count;
} cui_command_event_result;

cui_command_event_result cui_command_rebuild_events(
    cui_command_state *, cui_id target, const cui_command_entry *, size_t,
    cui_str query, bool query_changed, cui_command_options, cui_command_event_handler);
cui_command_event_result cui_command_select_events(
    cui_command_state *, cui_id target, cui_command_index, bool scroll,
    cui_command_options, cui_command_event_handler);
cui_command_event_result cui_command_move_events(
    cui_command_state *, cui_id target, bool forward,
    cui_command_options, cui_command_event_handler);
cui_command_event_result cui_command_confirm_events(
    cui_command_state *, cui_id target, cui_command_options, cui_command_event_handler);
cui_command_event_result cui_command_events(
    cui_context *, cui_id, const cui_theme *, cui_command_state *,
    const cui_command_entry *, size_t, cui_command_options, cui_command_event_handler);
```

Base has no context/ID today; the explicit nonzero `target` provides metadata
identity without retaining a context or inventing input provenance. Zero target
is an additive preflight failure: zero result, no reducer call/mutation/callback.
Other `control` values are exactly the old operation result. No `valid` or
`submitted` field conflates no-change/invalid/late-error paths. Counts saturate
with `count_overflow`, count only actual observer calls, and retain earlier calls
when a later Styled operation fails. No public observed clear helper or Base
cancel operation is proposed. Missing callbacks ignore `user`, including nonNULL.

### Callbackless predicates and frozen operation facts

The observer removes only the old callback-presence predicate, never the reducer
predicate. Keep old options callbacks and `options.user` unchanged; do not install
forwarders into those fields. Each pointer-free old callback precedes its new
observer. Borrowed query uses the ordering exception below. Freeze operation facts
before the old callback so redirects cannot turn
one operation into another. Observer snapshots describe the completed operation,
not a promise that current owner state still equals the payload after legacy code.

| Operation | Observer predicate independent of old callback presence | Order/facts |
| --- | --- | --- |
| Rebuild selection | Rebuild succeeds; `query_changed`; eligible selected path exists; no previous path or different path | Install all rebuilt rows/state, freeze previous/value, old `on_select`, new `on_selection` |
| Rebuild query | Rebuild succeeds; `query_changed && options.searchable` | After rebuilt state/path snapshots are installed, new `on_query` BEFORE legacy select/query hooks; old `on_query` keeps its original position after the selection pair |
| Select/move | Eligible item found and selected row index actually changes | Store selected/preserve/pending-scroll, freeze previous/value, old select, new selection |
| Confirm | Existing confirm returns true: state exists, options not disabled, selected row/item exists and item enabled | Freeze selected row/path, action callback and old confirm/user as today; action, captured old confirm, new activation |
| Styled cancel | Existing keyboard Escape branch reaches empty/nonsearchable query, enabled palette/focus, unconsumed unmodified KEY_DOWN; either old cancel or new cancel installed | Freeze REQUEST, old cancel if present, new cancel if present; no added consume/invalidate/host close |

Important exclusions from these exact predicates:

- `query_changed=false` rebuild may replace selection because an old row vanished
  or became disabled; it remains silent. `query_changed=true` need not imply
  different bytes in Base: it still reports query, even if the caller passes the
  same raw text. No selected-to-none event is invented when filtering yields no
  eligible item. `has_previous=false` makes `previous` absent (zero-filled).
- Styled first submission computes `changed=false` because `initialized=false`,
  so its initial highlight is silent. In contrast an explicit Base rebuild with
  `query_changed=true` can notify even on an uninitialized state. The earlier
  blanket “initial rebuild does not call hooks” refers only to Styled/false flag.
- Direct Base select/move do not check `options.disabled` or `state.loading`;
  preserve this policy. Confirm checks options/item disabled but not loading, and
  can activate without item action or legacy `on_confirm`. Same-item confirm is
  an ACTIVATE even when select first returned false. Repeated confirms are not
  deduplicated; pointer row interaction retains its existing aggregate coalescing.
- Never add a selection-loss callback, infer query previous text, replace query
  aggregation with per-edit events, or make new handler presence enable movement,
  selection, confirmation, consumption or focus. Only cancel gets the explicit
  new-handler-only notification branch described above.

### Initial, passive and input classification

| Actual seam | Metadata |
| --- | --- |
| Explicit Base operation, even called during a UI frame | PROGRAMMATIC, target only; no context/frame/index/activation invention |
| Styled leading rebuild detecting an externally modified editor hash | PROGRAMMATIC, target only, including disabled/loading/inert/layout-only/out-of-frame attempts |
| Styled initial or unchanged leading rebuild | Silent under the existing `changed=false` predicate |
| Aggregate query-input changed rebuild | UNKNOWN plus proven active frame only; do not choose an arbitrary input event/index or claim per-edit origin |
| Escape clear-query rebuild | Genuine keyboard frame/index/source from that loop event; CHANGE, no fabricated accepted activation |
| Hover-selected row | UNKNOWN plus proven frame; aggregate/stationary hover does not establish a causal event index or activation snapshot |
| Pointer/accepted row activation | Freeze before hover/select/content/binding callbacks; exactly one genuine accepted row activation supplies shared snapshot, multiple/missing evidence gives UNKNOWN plus proven frame only |
| Keyboard move/confirm/cancel loop | Freeze current event frame/index/KEYBOARD before callbacks; attach genuine accepted activation only if actually available for this operation/target, never copy a different target's snapshot |

All Styled targets are the Command root ID, while selection/activation payloads
carry exact section/row paths. Remapped accepted row evidence retains the actual
row target in `activation.target`; the root `event.target` is the owning Command.
Do not manufacture keyboard accepted snapshots for an Enter handled by the
palette loop when the accepted target is its query editor or nothing.

Only the leading externally changed rebuild is PROGRAMMATIC; a passive submission
is not universally silent. It still rebuilds before Styled focus/container and
geometry checks and may call old select/query hooks, so the new programmatic
observers must also run there. This includes disabled palettes and inert/layout-only
ancestry; suppressing those observers would lose actual model changes. Conversely,
interaction observations require nonzero `cui_frame_sequence(ui)` at their seam:
Core returns zero under inert/layout-only/measurement/out-of-frame work. Do not
add guards that suppress legacy callbacks/mutations merely to impose this observer
policy. Loading does not block existing nonempty-row navigation/confirmation;
retain it, rather than treating busy as disabled. Omitted submission emits nothing.

### Mutation, query lifetime and failure details

Capture accepted row evidence immediately after interaction/current semantic
registration and before hover selection or custom content/binding execution.
The later confirm operation freezes the **actual selected row at confirm entry**:
a legacy select callback may redirect it, and a content callback may change state.
Do not overwrite that confirmed path with the clicked row. If it differs, retain
only causal evidence that genuinely establishes origin, not an invented accepted
activation for the redirected item. Confirmation must snapshot its old confirm
callback/user before action, exactly as Base currently does; an action changing
selection does not change the ensuing captured confirm/typed activation path.
New observers must not mutate active state/editor/model/resources, recursively
invoke the same UI/reducer, or destroy context; legacy permitted redirects remain.

**Query-only ordering exception, requiring root review:** invoke the new query
observer after all rebuild state and selection facts are installed/frozen, before
the first potentially mutating legacy selection/query hook. Exact order when all
are eligible: new query -> old select -> new selection -> old query. If no select
transition is eligible, this reduces to new query immediately before old query.
Old select -> old query order, state installation, and their original argument
values remain unchanged; new observers cannot mutate active state/resources.

Placing the new query immediately after old query is unsafe: legitimate legacy
`on_query` can edit the editor and reallocate its backing store. Placing it only
immediately **before** old query, but after old select, is insufficient for
callbackless observation: legacy `on_select` can replace the raw storage while
`on_query` is NULL; the old implementation then never reads the raw query again.
Installing a new query observer there would introduce a new dangling-view read.
The proposed uniform pre-hook position avoids both hazards with no allocation,
workspace, refetching/reinterpreting query, or new legacy callback constraints.

The query view is exactly the original raw untrimmed rebuild argument, borrowed
only during the new callback, before legacy mutations. Never dereference the
saved view after either legacy hook returns. Observers needing retention call
`cui_component_event_copy_utf8` with explicit bound/storage and check its status;
struct copy alone is insufficient. Legacy on_select still has its **existing**
obligation to leave the raw argument valid if legacy on_query will subsequently
read it; this slice neither repairs that pre-existing ordering nor extends that
obligation when legacy on_query is absent. Legacy query editing/reallocation and
select redirects remain supported at their original times. No hidden snapshot,
owned-byte or post-hook view promise is made. Root must approve this earlier
query exception (or separately design explicit bounded caller snapshot workspace)
before production; a pointer freeze alone is not an acceptable solution.

Selection/activation/cancel payloads are pointer-free copies and survive context
teardown. All callbacks/users borrow only through the outer operation.

Leading rebuild can notify and install new rows before density/geometry validation
returns false. Input rebuild can notify before a custom measure failure. Retain
that progress and counts; do not attempt rollback, withhold notifications until
return, or relabel `control=false` as no work. Escape currently ignores rebuild's
bool before updating hash/consuming; preserve that legacy sequence and report only
actual successful reducer callbacks. No architecture/dispatch/teardown fixes here.

### Evidence and independently reviewed implementation sequence

1. Base only: shared types + four observed operations/private seams and tests.
   Verify callbackless predicates, explicit changed=true initialization, silent
   changed=false selection replacement/loss, select/move disabled policy, confirm
   without action/hooks, redirects and action snapshot, raw query/copy bounds,
   counts/zero-target failure and unchanged old bool returns. Add a legacy query
   hook that grows/reallocates its editor after the new observer copies the view,
   plus a legacy select hook doing the same with old query absent. Assert old
   callbacks retain their relative order and no post-hook view is accessed under
   ASan. Cover old query present/select redirect with the same original raw-view
   obligations, without silently forbidding legacy edits. No Styled activation
   claim from this slice.
2. Styled submission: same private reducers with scoped cause contexts; leading
   programmatic rebuild, aggregate query, hover/row confirm, keyboard/cancel.
   Compare old/new paint/semantics/focus/consume and callback traces, including
   nil legacy hooks, old hooks consuming input/changing selection, stationary
   hover, actual accepted pointer/accessibility eligibility, repeated keyboard
   confirms, disabled vs loading, inert/layout-only/measurement and omitted work,
   changed editor before passive submission, late custom geometry failure.

Source anchors: [Base rebuild](../../src/base/cui_command.c#L83) installs state
then select/query at lines 172–178; [select/move/confirm](../../src/base/cui_command.c#L181)
show selection predicates and captured confirm at 223–228. [Styled entry](../../src/components/cui_command.c#L76)
rebuilds before geometry; aggregate query at 163–180, hover at 228–232, accepted row
at 291–295, footer then keyboard at 309–335. [Core frame identity](../../src/core/cui.c#L4463)
excludes inert/layout-only work. Existing [model tests](../../tests/unit/test_command.c#L40)
cover callback ordering, clear/preserve, filtering/disabled selection and capacity;
[render tests](../../tests/unit/test_command.c#L165) cover stationary hover, click
focus, Escape and disabled consumption; [slots/loading](../../tests/unit/test_command.c#L392)
cover presentation/busy, not suppression of confirmation. Existing tests do not
prove legacy action redirects, passive observer classification or new metadata;
the new focused tests above must supply that proof. No builds run for this
checkpoint and no foreign binding coverage is claimed.

### Base implementation checkpoint

The independent Base slice implements only rebuild/select/move/confirm event
entrypoints, selection/query/activation payloads, handler fields and counts.
Cancel types/handler/count and Styled declaration are deferred until that slice is
approved; no reserved silently ignored public field ships here. Early query
exception is implemented after installation/all path snapshots and before legacy
hooks. Actual editor detach/realloc/recreate regressions cover oldselect with
oldquery NULL and oldquery itself, with observer-owned copies before either edit;
ASan additionally checks allocation actually moved. Legacy-only/new paired traces
preserve redirects, action identity and old bool/state semantics. No Styled,
Core, dispatch, build wiring, generated SDK or binding edits in this slice.

### Styled implementation checkpoint

Implemented independently after Base review: private cause context shared through
`src/base/cui_command_events_private.h`, with globally unique
`cui_command_events_private_*` seams for amalgamation. Public Base entrypoints
still create only contextless PROGRAMMATIC observers. Styled uses the same actual
reducers once, preserving early query ordering, legacy paint/callback/focus/consume,
leading passive PROGRAMMATIC rebuilds and late-error progress. Added implemented
cancel type/field/count and Styled declaration, no reserved field.

Focused paired tests cover keyboard multiplicity, aggregate query/clear/cancel,
custom content consuming pointer evidence, legacy selection redirects and actual
confirmed identity, unique/ambiguous row activations, actual OPTION accessibility
FOCUS/ACTIVATE eligibility, disabled/loading/inert/layout-only, leading and input
rebuild notifications before geometry failure. Existing Base realloc regressions
remain in the same strict/sanitizer test. No Core/dispatch/role/wiring/generator or
foreign schema changes. Root must inline the new private header in amalgamation.
