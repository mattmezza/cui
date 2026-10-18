# Settings event checkpoint

Design only, audited at `ace7190d`. No declarations below are shipped APIs.
Preserve the shared component event contract and all existing Settings behavior.

## Existing ownership and callback contract

`cui_settings.h` borrows page/group/item arrays, labels, mutable value pointers,
editors, control states, dropdown workspaces, custom render/dirty/reset/search
callbacks and userdata. `cui_settings_state` borrows page states and optional
search editor. Source indices survive filtering; they are positions rather than
persistent identities across a caller reorder. Derived control IDs use
`cui_settings_part_id` with the compound root and source indices.

`cui_settings_filter` validates nested arrays before initialization, chooses the
initial page/group, counts matching entries, and reconciles hidden selections.
It can call custom `matches` repeatedly. Initial default selection is established
before its previous-value comparison. No input evidence exists for this helper;
filtering is maintenance, not user navigation or value acceptance.

Styled `cui_settings` filters before layout, and filters again immediately after
search reduction so same-frame text affects navigation and content. Navigation
uses a synchronous private Sidebar `on_activate` callback: set result flag,
write page/group, set pending group scroll, invalidate. It is distinct from
filter reconciliation. Layout can be stacked or split; the split path records
final sidebar width after Resizable and content callbacks return. Search,
scrolling, resizing and selection are distinct domains.

Built-in field adapters run once in content order. After reducing/painting a
field, they record the first edit error, then invoke existing item `on_change`
when its old `changed` predicate is true. This predicate is component-specific:
Input uses its aggregate changed flag, Checkbox writes its local enum back to
the owner, Number additionally applies bounds and updates `last_value`, Dropdown
uses Menu or legacy Select. Custom `render` bypasses the built-in adapter and
produces no built-in changed result. Presentation callbacks are not event handlers.

`cui_settings_reset_page` visits matching items in source order. Custom reset
runs instead of the built-in path and returns success without `on_change`.
Built-in defaults write/reset, then call `on_change` on successful operation,
even if a scalar was already equal. Disabled items are not excluded by this
helper. A later malformed group can return false after earlier resets; there is
no rollback. Styled reset calls this helper from the existing reset button.
Do not infer value change or suppression from its aggregate bool.

## Independently reviewable implementation slices

1. **Navigation only.** Add `cui_settings_events` via a narrow Styled common
   entry; legacy entry supplies no observer. New handler has only implemented
   `on_navigation` plus `user`. Payload is pointer-free shared metadata,
   `{previous_page, previous_group, page, group}`. Notify immediately after the
   existing navigation writes/invalidation when the pair actually differs.
   Preserve repeated/net-zero navigation and actual callback counts separately
   from unchanged `cui_settings_result`. Freeze genuine Sidebar activation
   evidence before callback writes when available; otherwise use only evidence
   actually retained by that seam. No invented local input index or source.
   Filter initialization/reconciliation is silent; do not report it as navigation.
   Identical-page activation can still update pending scroll under the old path
   without a navigation CHANGE. Require new compound ID and normal required
   arguments before any new submission; missing handler preserves all old work.
2. **Built-in scalar values.** Separate typed boolean, double and choice-ID
   handlers with page/group/item indices and derived field ID. Capture entry
   values immediately before the old adapter, invoke old `on_change` in exactly
   its current position, then read the supported final owner and report actual
   inequality. This intentionally observes callback-driven legacy redirection or
   cancellation. It is an applied notification, never a rejectable request.
   No new side-effect replay when reading result counts. Each adapter is an
   aggregate operation; do not claim per-input multiplicity inside nested controls.
   Preserve partial-error changes and Number's additional bounds policy.
3. **Built-in text values.** Separate proposal revision before production:
   use editor revision and borrowed final UTF-8/length with caller-controlled
   copying. A previous text snapshot requires explicit bounded caller workspace;
   do not allocate an unbounded hidden before-copy or claim struct copies own text.
   Audit legacy callback-driven editor mutation and existing Input error paths.
   Settings has no secure-input configuration; do not infer redaction support
   from the separate direct Input facade.
4. **Reset observations.** Separate helper/common reset seams, preserving source
   order, custom reset behavior, same-value old callbacks, disabled behavior and
   failure after progress. Reuse typed value comparisons where supported, with
   explicit operation/reason metadata distinguishing reset from field editing.
   Custom reset cannot produce a typed value without a separate host contract.
   Programmatic reset has UNKNOWN source/no fabricated input or activation. Do
   not call reset success ACTIVATE or COMMIT merely because a button may invoke it.
5. **Search, resize and viewport.** Audit independently. Search can reuse its
   actual text revision semantics; width/scroll may need honest aggregate final
   completion observations. Do not turn filter maintenance into user navigation,
   final geometry into per-input movement, or render callbacks into change events.

No slice changes old public struct layouts, nested state ownership, dispatch,
input consumption, acceptance rules, or dependency direction. New typed handlers
are synchronous and borrowed for submission. They may copy pointer-free payloads
and modify unrelated application flags, but may not mutate/rebind/free active
page arrays, values/editors/states, dropdown workspace, labels, callback userdata,
input or context. Legacy callbacks keep their existing mutation timing and
behavior; they must still preserve storage required by the remainder of the old
operation. Omit/remove/destroy after outer return and renderer consumption.
No context destruction or recursive UI submission during dispatch.

## Evidence required before each slice ships

Keep `tests/unit/test_settings.c`: filter, navigation/fields, search/reset/resize/
scroll, input/number/sizes, Menu dropdown/reset tooltip, design sizes and narrow
layout. Add dedicated headless event tests with paired old/new state, owner
values, legacy results, callbacks, focus, semantics and draw output. Cover ordered
navigation and net-zero transitions, repeated activation, filter reconciliation
silence, custom callbacks, partial errors, callback-driven scalar redirects,
copy/deferred omission, unsupported active teardown, null observer, disabled and
passive paths. Test actual pointer/keyboard/accessibility routing supported by
Sidebar; never fabricate accepted evidence or promise support from compilation.
Native Linux runs through `python scripts/test_local.py --build-dir build`.
No desktop windows. Strict C11/C++ headers and warnings are required.

## Binding omissions

`bridge_render_settings` builds separate owned pages and field callbacks, then
calls old Styled Settings. Field callbacks enqueue existing generic bridge
changes; final page selection is compared after the complete operation. Errors
return before final page notification. Existing Python/JS/TS/Go/Rust wrappers do
not expose this proposed immediate navigation or field observer, source indices,
per-navigation counts, reset reasons or synchronous requests. New opt-in scalar/
text polling does not automatically classify Settings or its nested item slots.
Document and test any bridge extension separately, without changing old queue
order, after-mutation semantics or foreign lifetime contracts.

## Navigation implementation checkpoint

The first slice implements only Styled `cui_settings_events` navigation. Its
private per-trigger descriptor uses a monotonic actual-activation lookup matching
Sidebar's synchronous per-item callback loop; accepted snapshots are copied before
writes. This is evidence lookup only, without Sidebar modification or replay.
Unexpected mismatch retains only proven frame/UNKNOWN evidence. Notification follows
original pair/pending-scroll writes and invalidation, before content. Existing item
traversal order (not global event order), filter repairs and all field/reset paths
are preserved. No unimplemented scalar/text/reset handler fields were added.


## Scalar implementation checkpoint

The second slice adds only boolean/double/choice-ID field observers. Each existing
built-in adapter captures entry immediately before reduction, preserves error and
old on_change timing, then observes actual final owner inequality. Number finite
flags report nonfinite callback redirects; two NaNs and signed zeros are equal.
Disabled/passive/inert maintenance remains unobserved without changing old writes
or callbacks. Aggregate field metadata is frame-only UNKNOWN, with compound target
and separate derived field ID/source indices. No input reconstruction or text/reset
observer was added; custom render overrides remain outside built-in scalar hooks.
New handler/result fields require consumer rebuild; old legacy layouts stay intact.
