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
3. **Built-in text values.** The revised exact proposal below supersedes the
   earlier before-text snapshot/workspace option. Observe committed revision and
   byte-count snapshots plus callback-borrowed final UTF-8 after old callbacks.
   No hidden or caller-required before-text copy, per-input multiplicity or SUBMIT.
4. **Reset observations.** The revised proposal below reuses the implemented typed
   value observers with explicit EDIT/RESET reasons; no invented reset action or
   lifecycle event. Preserve all existing helper/callback/order/failure behavior.
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


## Revised text/reset checkpoint — design only

This section proposes the next slices; it does not declare shipped APIs. The
navigation/scalar checkpoints above describe their prior implemented scope. No
production, generated or binding files change in this revision.

### Exact text API and compatibility

In `cui_settings_events.h`, propose:

```c
typedef enum {
    CUI_SETTINGS_CHANGE_EDIT = 0,
    CUI_SETTINGS_CHANGE_RESET
} cui_settings_change_reason;

typedef struct {
    cui_component_event_metadata event;
    cui_id field_id;
    size_t page, group, item;
    uint64_t before_revision, after_revision;
    size_t before_bytes, after_bytes;
    cui_str text; /* final committed UTF-8, borrowed only through this callback */
    cui_settings_change_reason reason;
} cui_settings_text_change;
/* Append to the new handler: */
/* void (*on_text)(void *, const cui_settings_text_change *); */
/* Append to the new event result: size_t text_count; */
```

Append `reason` to the existing new bool/number/choice payloads as well. Existing
adapter notifications use EDIT; reset observations use RESET. Navigation has no
reason change. These event structs/handler/result are **unreleased** additions:
changing their ABI requires rebuilding all consumers/generated copies before a
release. Use designated initializers. EDIT=0 preserves zero/default interpretation;
there is no claim of binary compatibility with the preceding event-struct layout.
Do not change old Settings functions/options/state/result or old callback types.
Add fields only with the independently implemented slice, not reserved stubs.

`cui_settings_events` keeps its signature; on_text shares the existing handler user.
Capture the Input editor's `cui_edit_get_state` revision/length immediately before
the old built-in adapter. Run the same Input once, preserve its aggregate error and
old on_change predicate/timing, then read the **same still-live editor's** final
state and `cui_edit_get_text`. Emit zero/one text CHANGE if revisions differ, even
when old changed is false or a later edit error exists. The final byte count equals
text.length; before_bytes is the old committed UTF-8 length, not characters/caret.
Use revision inequality, not ordering/subtraction. Multiple changes/undo/redo can
end with identical bytes but a different revision; that is one revision-change
notification, not proof of differing old/final strings. No before string, changed
range, selection transaction, request, COMMIT or SUBMIT is promised.

The callback follows old on_change, so it observes callback-driven committed edits
and redirects, including undo to the prior bytes. Only actual revision difference
matters. Selection/scroll/caret changes and IME preedit without committed revision
change are silent; commit/undo/redo may notify. Do not reposition old paint or
rerender to match callback-redirection text. Preserve the first aggregate edit_error
exactly; unchanged revision on failure is silent, changed revision before/through
an error is still notified. Legacy callback edit errors are not reconstructible
from its void return and must not be invented in the payload/result.

EDIT metadata stays frame-only UNKNOWN, compound target plus explicit field ID
and source indices. A whole Input plus host callback is not a unique input event.
Enabled only: preserve disabled/passive/inert old work while suppressing new EDIT
notifications. Custom render overrides remain outside built-in text observation.
Settings supplies no secure/masked Input option; this proposal exposes committed
bytes and adds no implicit password redaction or before-text retention.

Payload pointer/text are borrowed only during callback. Copy text using existing
`cui_component_event_copy_utf8` with caller maximum/capacity; failure is atomic,
no truncation/allocation. Copying the struct retains only a borrowed string pointer.
Legacy callbacks must leave the editor/owner storage readable through the typed
notification, and must not replace/destroy the editor while the opted-in operation
is active. Typed observers cannot edit/rebind/free active resources, submit UI,
change input or destroy context. Unrelated flags allowed; defer omission/teardown
until outer return plus frame/render consumption. No new runtime guard is implied.

### Reset helper and exact observation order

Current authority is `src/base/cui_settings.c:reset_item/cui_settings_reset_page`:
custom reset wins first and returns true without old on_change; otherwise missing
default/custom_item returns false. Built-in bool/double/choice writes and successful
text replace call old on_change even if the value/revision was already unchanged.
`cui_edit_replace_range` can accept same bytes without revision increment (while
changing selection). Matching is evaluated once per visited item in source order;
disabled is not a reset exclusion. A malformed later group's nonnull-count/null-
items case immediately returns false, discarding the aggregate earlier-success bool
but not earlier writes/effects. Failed item text replacements return false and do
not call old on_change; subsequent items still run. No transaction/rollback exists.

Propose an additive helper using the **same** typed field handlers:

```c
typedef struct {
    bool reset; /* exact legacy helper bool, including false after progress */
    size_t boolean_count, number_count, choice_count, text_count;
    bool count_overflow;
} cui_settings_reset_event_result;
cui_settings_reset_event_result cui_settings_reset_page_events(
    cui_id root, size_t page_index, const cui_setting_page *page,
    cui_str query, cui_settings_handler handler);
```

Root and page_index are necessary: the old single-page pointer has neither a root
identity nor its position in a host page array. Require nonzero root and nonnull
page before invoking the old common helper; invalid new admission returns a zero
result. The caller supplies the truthful source page index; no invented zero index,
global ID lookup, artificial page cap or context is introduced. Group/item indices
come from the existing loop. Preserve old page/query/no_reset validation and all
matching calls. Missing handlers still execute the same resets; navigation handler
is ignored by this helper. Keep old helper signature/return and no-observer path.

At each matching **built-in** reset, capture the corresponding scalar or text
revision/byte snapshot, run exactly the existing reset and old on_change sequence,
then notify actual final inequality/revision difference with reason RESET. Reuse
the same equality, finite flags and typed payloads; Number reset does not introduce
bounds checks or update number_state.last_value. Same-value successful old callbacks
still run: no typed event if final value remains equal, but callback redirect to a
different final owner does notify. Text replacement rejection with no revision
change emits none. Do not gate observation on helper aggregate bool or operation
success: delivered callbacks/counts survive a later false return. This helper
retains only the old reset bool, not a fabricated per-item error/atomicity guarantee.

A custom reset stays an explicit typed-value omission even if an item also has a
scalar pointer: arbitrary host reset may affect multiple owners or replace its
own model, and its opaque callback has no typed return/lifetime contract. Invoke
it once, preserve true/no-old-on_change, emit no built-in typed RESET. Custom render
alone does not override reset: when reset==NULL, has_default is true and custom_item
is false, existing built-in reset still applies and can be observed. No-default
and unmatched items remain untouched/silent. There is no reset ACTIVATE, LIFECYCLE,
COMMIT, synthesized request or new standalone reset callback.

Programmatic helper metadata is known PROGRAMMATIC source, compound target, **no
frame/index/activation** (the API has no context). Typed RESET intentionally reports
actual built-in changes to disabled items too: explicit reset already includes them;
this is not a disabled-input event. This differs deliberately from EDIT suppression.
Custom reset's omission does not alter reset bool, old effects or source order.

### Styled reset composition

Use a private common reset implementation/evidence argument, not a call to the new
programmatic public helper that would mislabel UI reset as PROGRAMMATIC. Existing
Styled button_ex runs once at the same header point; keep result.reset assignment
and complete all matching resets before old content proceeds. Freeze cause before
the first reset write: if there is exactly one actual matching button activation
and its genuine accepted snapshot is available, copy it, retaining the reset
button's original activation.target while event.target remains Settings. Multiple
coalesced activations or unavailable evidence get only proven active frame/UNKNOWN,
never a guessed first/last event. No early registration/replay/consume. New metadata
kind is CHANGE, reason RESET; accepted activation is supporting evidence only.

Suppress new Styled RESET observations for passive/layout-only/inert work while
preserving any old writes/callbacks. Active reset still includes disabled items.
Reset is before the field adapters. Therefore default Number20 can produce RESET
3→20, then the old later Number bounds pass can produce EDIT20→10 in the same call;
text formatting/host redirects can similarly create later adapter work. Preserve
and document these ordered operations rather than collapsing them or mutating
last_value/drafts early. Navigation counts/results stay independent. Reset errors
are not added to old Settings edit_error: only existing adapters set that field.

### Independent proof and proposed ownership

Implement text EDIT first; then reset helper/Styled composition in a separate
verified commit. Extend only the existing Settings event header/common Base reset
seam/Styled seams/test/guide/proposal and discovery comment after approval; root
owns wiring/generation. No Core/Input/editor mutation redesign or bridge change.

Pair old/new complete operation outputs/state/callback counts and paint timing.
Text tests: real text/paste/multi-input, IME preedit/commit/cancel, undo/redo and
same-byte revised text, legacy on_change redirect, failure after earlier committed
mutation, unchanged failure/selection-only silence, null handler, disabled/passive/
inert, custom render omission, exact UTF-8 byte counts, bounded copy success/failure,
borrowed pointer versus owned copy, deferred omission and model lifetime limits.

Reset tests: programmatic no-frame PROGRAMMATIC metadata; Styled unique real snapshot
versus multi-activation honest fallback; source-index IDs; match order/disabled/
no-default/no_reset policy; built-in same-value old callbacks with and without
redirect; text acceptance/presentation/capacity failures followed by later items;
custom reset true/no-old-on_change/no-typed-event; malformed group after earlier
observations returning false with retained counts; Number nonfinite defaults/finite
flags/unchanged state synchronization; same-frame RESET then EDIT order; copied
scalar/text payload lifetimes; null handlers and passive UI suppression. Existing
Settings/Input/Number/Choice/edit acceptance regressions remain mandatory through
the private wrapper, plus strict C/C++ headers. No foreign wrapper or full parity
claim follows from those checks. No builds or production changes in this revision.
