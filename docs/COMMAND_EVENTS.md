# Command observers

Include `cui/cui_command_events.h`. The additive `cui_command_rebuild_events`,
`cui_command_select_events`, `cui_command_move_events` and
`cui_command_confirm_events` accept the existing arguments, an explicit nonzero
metadata target after state, and a `cui_command_event_handler`. Existing functions
and public layouts remain unchanged. Base and Styled submission are available; foreign bindings remain separate work.

```c
static void confirmed(void *user, const cui_command_activation *event) {
    cui_command_index *saved = user;
    *saved = event->value; /* Copy the confirmed path, not later selected state. */
}
/* state/options/selected_path are application-owned. */
/* cui_command_confirm_events(state, 90, options,
 *     (cui_command_event_handler){.on_activate=confirmed,.user=&selected_path}); */
```

`result.control` is the exact legacy bool result: false can mean no change or
invalid input, and does not imply rollback. Zero target rejects before any reducer
work. Counts record actual synchronous observer calls, saturating with
`count_overflow`; absent callbacks ignore user and preserve reducer behavior.
Base metadata is PROGRAMMATIC with the supplied target, without invented context,
frame, index or activation evidence, even when the application calls it in a UI
frame. Section/row indices are operation-time paths, not stable item IDs.

Selection CHANGE follows each successful select/move transition or changed-query
rebuild that installs a different eligible path. `has_previous` establishes whether
`previous` exists. Rebuilds with `query_changed=false` remain silent even if model
replacement removes/changes selection. Selection loss emits no new event.
Explicit Base `query_changed=true` can notify on initialization, unlike Styled's
initial false flag; it need not mean different query bytes. Direct select/move
retain the legacy policy of ignoring options.disabled/loading. Confirm respects
options/item disabled, but loading and missing action/hooks do not suppress it.

Rebuild installs state and freezes all selected-path snapshots, then invokes the
new query observer **before either legacy hook**. When all are eligible the order
is new query → old select → new selection → old query. The old hooks retain their
relative order and timing. Query observation requires changed && searchable and
borrows the original untrimmed raw query view only during callback. This explicit
ordering exception lets old select/query hooks replace/reallocate query storage
without adding a dangling-view read afterward. The old select hook retains only
its pre-existing obligation to preserve the view for old on_query when installed.
No allocation, workspace or hidden query snapshot is added. To retain text use
`cui_component_event_copy_utf8` with explicit bound/storage and check its status;
a struct copy does not own text.

Other new observers follow the old hook. Select freezes paths before legacy code
can redirect state. Confirm snapshots actual selected row and legacy confirm/user,
then runs action → captured old confirm → new activation. Action-induced selection
changes do not rewrite the confirmed path. Same-item/repeated confirmations remain
separate ACTIVATE calls. No callback rejects an operation, changes consumption or
introduces commit/submit semantics.

Handlers/user borrow through the outer call. Selection/activation payloads are
pointer-free copies, safe to retain beyond resource teardown. New handlers may
copy facts or set unrelated application flags; they must not mutate/rebind active
state/editor/model/resources, recurse into the same operation, destroy resources
or unwind across C. Legacy mutations retain their existing timing. Defer removal
and teardown until the outer operation and any renderer consumption complete.
Missing callbacks do not make controls read-only. Clearing selection remains the
existing silent helper. Styled behavior is specified below; Base always remains contextless.

## Styled submission and causes

`cui_command_events(ui, id, theme, state, entries, count, options, handler)`
returns the same typed result with the legacy Styled bool in `control` and counts
across all operations actually observed during that call. No final-state replay,
extra model interaction or allocation is added. A later false return retains
successful earlier query/selection notifications, including before invalid custom
geometry. The same early-query exception and callback lifetime rules apply.

The leading rebuild detects an externally edited query by the existing hash. It
is PROGRAMMATIC with no invented frame/input evidence, including under disabled,
inert/layout-only ancestry or loading. Initial/unchanged rebuild remains silent.
An aggregate query-input change reports UNKNOWN with the proven active frame,
without picking a text event/index; Escape query-clear has its actual keyboard
frame/index/source. Hover selection is aggregate and may be stationary: it reports
UNKNOWN/frame-only, not a fabricated pointer event. No per-edit query claim.

A row's activation evidence is captured after its current semantic interaction,
before hover-selection or custom content/binding/old callbacks can consume input
or redirect state. One genuine accepted activation supplies its exact snapshot;
multiple/missing evidence reports frame-only UNKNOWN. Shared event target is the
Command root; the snapshot keeps its actual row target. Confirm snapshots the
actual selected path at confirm entry, including legacy redirects. If it differs
from the clicked row, the copied clicked-row activation snapshot is omitted;
known causal input index/source remains. Action changes after confirm entry do
not rewrite that identity. Keyboard loop observations have actual frame/index/
KEYBOARD evidence, without inventing accepted activation for a query-editor target.

`on_cancel(user, const cui_command_cancel_request *)` reports REQUEST to dismiss
the host, not completed dismissal. It runs after legacy on_cancel, or independently
when only the new observer is installed, on the same eligible empty-query Escape
branch. `cancel_request_count` counts actual observer calls. No additional consume,
invalidation, overlay close or veto is introduced. Nonempty searchable Escape
still clears/rebuilds query rather than requesting cancellation.

UI observations require a nonzero ordinary active frame; inert/layout-only,
measurement and out-of-frame work emit no new UI callbacks. Legacy callback and
mutation rules remain unchanged. Disabled suppresses UI interaction; loading is
busy presentation and does not suppress existing nonempty-row confirmation.
No submission emits nothing and registers no callback. Command rows retain OPTION with the existing pointer_activatable detail; actual
accessibility FOCUS and ACTIVATE are supported without a role remap. This does not
claim broader foreign/platform accessibility parity. Custom content, matching and binding
callbacks remain separate APIs. Native synchronous observers do not add bridge or
Python/Node/Go/Rust event categories.
