# TimeField focus routing decision

Design checkpoint for one-stop TimeField navigation. The production baseline is
`d7e9e9d2`. The checkpoint includes an executable trace for the existing route.

## Reference behavior

The field owns one focus handle. Visible segments own selection and pointer hit
geometry. Tab and Shift-Tab select the next or previous segment. At the last or
first segment, the corresponding key uses the surrounding focus order. Entry
in either direction resumes the field's selected segment. A digit can finish a
segment, and that new selection applies to the next input even before the next
paint. The source sets a formatted value on a root time-input semantic.
[Frozen Base implementation](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/time_field.rs#L266-L383)
· [Frozen root and pointer implementation](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/time_field.rs#L532-L585).

## Reproduced ordering constraint

The established CUI route resolves a batch against completed semantics and
geometry during `cui_begin_frame`. Applications can inspect its targets and
consume inputs during submission. TimeField then edits its caller's state and
value, updates semantics and paint, and synchronously reports its aggregate
change. Updating focus during that edit cannot change an already targeted key
or an activation already offered to a sibling. See [Core dispatch](../../src/core/cui.c),
[TimeField reduction](../../src/base/cui_time_field_v1.c), and the
[`tab_routing_checkpoint` fixture](../../tests/unit/test_time_field_v1.c).

The fixture starts at Hour with a minute-precision value of `09:30`. Typing
`3` completes the hour immediately and selects Minute. A button follows the
field in ordinary focus order.

| Input delivery | Observed result |
| --- | --- |
| `3`, Tab, Enter in one batch | Hour becomes `03`. Core already routed Tab from Hour to Minute, so Enter targets Minute and the button stays idle. |
| Process `3` first, then deliver Tab and Enter together | Hour becomes `03`. Tab leaves from Minute and Enter activates the button. This serial trace gives the expected ordered result for the proposed route. |
| `3`, Tab, Enter in one batch, with `3` consumed before widget submission | The time stays `09:30`. Tab reaches Minute and the button stays idle. An early projection that assumes the edit goes through would choose the wrong exit. |
| Process `3` with a minimum time of `09:00`, then deliver Tab and Enter | The field rejects the value `03:30` and keeps `09:30`. Completing the digit still selects Minute. The following Tab leaves the field and Enter activates the button. Value rejection and pre-reducer consumption have different routing consequences. |

The same result holds for Base and Styled. Existing per-segment navigation and
its event targets remain covered by the compatibility tests.

## Alternatives

| Approach | Consequence |
| --- | --- |
| Use existing tab groups, the focus delegate and `handles_tab` | They can describe membership, selection of a pointer focus target, and whether a control handles Tab. They cannot choose a boundary after earlier edits in the same already-routed batch. |
| Apply a boundary focus change during component submission | A later key, release or AT action may already have reached another control. Rewriting targets at the field would make observations depend on sibling submission order. |
| Draw or submit a frame after each input | The serial trace is a useful oracle. Making it the host default changes batching, callback aggregation, omission processing and frame work throughout CUI. |
| Predict the segment from the last frame's value and queued inputs | Current consumption, value, mode and presence can differ. The consumed-digit trace is a minimal counterexample. |
| Opt in to ordered input participation | The opted-in control applies each admitted edit to a bounded working copy before routing the next event. The existing immediate batch route remains available. A new participation and mutation contract is required. |

## Proposed architecture

Use an **opt-in ordered input path** for a single compound control. Review the
following boundary before writing production dispatch code.

1. The application explicitly admits participants before dispatch, with the
   current typed model/options, enabled policy and an ID/generation that ties
   each participant to its completed hit geometry. Admission is a commitment
   for that batch. Owner changes caused during later composition apply at the
   next admission. Ordinary controls retain their current submission rules.
2. The input loop resolves actual targets in sequence. For an admitted
   TimeField, run the existing editor rules against its typed working copy
   exactly once. Give the root one global stop. Keep hit segments and the
   selected segment distinct from the root's keyboard focus. Internal Tab
   changes selection. Boundary Tab asks normal traversal to continue from the
   root, respecting modal scopes and direction. The next event uses the focus
   and selection at that point in the sequence.
3. Any application filter that can veto an enrolled edit must act before that
   edit influences later routing. A composition-time consumption observation
   cannot undo completed routing. An ordered participant's prevention of a host
   action, consumption of input and acceptance of a typed value are separate.
   This path has no general capture or bubble phase.
4. Dispatch does no application notification or layout. It returns a typed,
   bounded outcome for the current operation. The new component route applies
   it once and then follows the synchronous semantics/paint/observer order.
   Neither frame results nor additional reads apply outcomes again. Bind state
   and outcomes to the context, admitted identity and batch generation.
   Borrow producer data only through dispatch. Copy the data needed afterward
   into bounded storage. Establish failure and teardown behavior before exposing
   the path. Reserve workspace before admission, reject invalid admissions,
   and disallow stale or repeated application of outcomes.
5. Hosts must be able to take the browser's synchronous Tab default decision
   from the ordered navigation result, including a canvas edge that should
   leave focus to the document. Keep the logical root distinct from the
   segment that owns a composition session and candidate anchor. Native and web
   hosts need to reject stale segment commits after departure or movement.
   Preserve the current SDL empty-edit/late-commit handoff on existing routes.

The proposed admission and mutation timing is observable and belongs to a
separate versioned opt-in. The existing APIs, C11 dependency boundaries,
segment IDs, callback timing and consumption rules form the compatibility
reference. Root accessibility semantics and adapter mappings need a separate
review, including a formatted value and focus/selection evidence. This
checkpoint records the proposed behavior and the compatibility tests.

## Implementation and acceptance gates

1. Build a deterministic headless routing harness for mixed one-batch edits,
   exits, re-entry and sibling activation. Include early consumption and
   rejection independently, changed modes, modal scope and previous geometry,
   invalid admissions, and failure without replay. Compare the frozen serial
   rule with the existing V1 fixture.
2. Review typed participant setup, bounded workspace, reset/recovery and
   callback order before introducing the opt-in C path. Test both directions,
   internal and boundary repeat policy, pointer and accessibility selection,
   two independent fields, inactive/omitted widgets, teardown and stable
   legacy target and callback evidence. Qualify digit draft behavior across
   focus departure separately from cancellation of a live IME composition.
3. Verify native and browser routes, especially real browser Tab to and from
   neighboring DOM controls, disabled exclusion, touch selection and IME
   geometry. Publish the new coverage only after the C route, gallery and
   applicable bindings have evidence. Physical input and screen-reader
   acceptance require their own checks.

## Private Core experiment

The [same-translation-unit test](../../tests/unit/test_ordered_time_field_routing.c)
drives the actual Core event queue and the V1 localized digit reducer. It passes
one temporary admission to the internal frame entry point. The root uses the
existing global stop and delegates child pointer focus to itself. The reducer
uses a typed stack copy, with separate decisions for consuming input and
handling an internal Tab. Core performs normal target selection, traversal and
activation evidence recording. The two public frame entry points use the
ordinary path.

The trace covers a digit and boundary Tab reaching the following button with
the original Enter snapshot, a consumed digit keeping Tab inside the field,
and a rejected value still advancing selection. It also checks reverse exit,
re-entry retaining selection, real segment hits, accessibility focus on the
root, modifiers and composition guards, and opt-in rejection for missing,
disabled and modal-blocked prior roots. An empty follow-up checks for replay.
The failure fixture discards the working copy after an invalidated frame and
checks recovery and absence of replay. A change to minute precision checks
that a formerly visible Second hit cannot select the now-hidden segment.

## Direct C implementation

The [localized guide](../TIME_FIELD_LOCALIZED.md#one-tab-stop-per-field) defines
admission, borrowing, dispatch and state application. The public tests exercise
two fields in one batch, submit them in reverse order, and check a real sibling
activation. They also cover a Styled segment hit, disabled and omitted roots,
failed admission, and discarding an edit before application. Layout or rendering
can fail after a synchronous observer has acted. The C route has a headless
[two-field example](../../examples/time_field_ordered.c).

## Gallery host handoff

The gallery offers an ordered schedule preview alongside the per-segment IME
preview. On an ordinary Tab keydown the browser host tags one queued key and
runs the current ordered frame synchronously. Core sees any earlier input in
the batch. Normal traversal stays within the preview until it runs out of
stops in that direction. At an open edge Core clears scene focus and reports
that the browser can run its native Tab default. Modal scopes continue
wrapping. Consumption and the host handoff are distinct decisions. This
policy is private to the bundled host. Public C ordered frames retain normal
CUI traversal.

Headless Chromium tests real focus transfer at the canvas edges and retains a
pending digit across Tab. Ordered IME, platform accessibility mapping and
foreign authoring remain open.
