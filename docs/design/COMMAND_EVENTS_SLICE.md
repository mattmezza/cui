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
   legacy callback seam freezes payload before legacy callback, invokes that
   callback with original userdata, then invokes the borrowed observer. Confirm
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
veto. CHANGE/ACTIVATE/REQUEST are notifications after the documented legacy seam;
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
