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
pending digit across Tab.

## Ordered composition

An editable segment can own the finalized host text session while Core focus
remains on the root. The additive queue entry point admits an event only for
the published segment and generation. Core uses a bit per queue slot to
identify these offers. The existing event struct, ordinary host admission and
activation evidence retain their existing shapes. In ordered dispatch a
focus or segment change invalidates the previous session before later events
can reach the reducer. A transition away and back generates a new session.
The reducer copies bounded preedit state and publishes it when the field
submits. A retained draft also matches the host generation.

SDL captures the pair for composition and its empty-clear/late-commit handoff.
The gallery's transient text proxy captures its segment and generation at
creation. A replaced proxy drops its remaining browser events and deferred
selection reads. Native browser text and composition use the generation-checked
queue. Physical key events use the ordinary ordered queue so a pending digit
can advance the segment before a queued Tab.

The ordered proxy sits beside the canvas in the DOM. When Core reaches a
non-text widget or clears focus at an open edge, the browser host restores
canvas focus before removing the proxy. Headless Chromium tests composition,
candidate alignment and stale proxy input. Chromium and Firefox verify real
Tab in both directions with the proxy active. Physical IME, accessibility
editing and foreign authoring remain open.

## Accessibility projection checkpoint

At `35b76932`, the Chromium accessibility tree focuses a textbox named
“Canvas text input” with an empty value. The mirrored Hour and Minute nodes
expose their button text as a value. The AccessKit mapping similarly lacks
the committed textual value. The frozen reference publishes the formatted
time on a `TimeInput` root. The ordered C model already provides the
information needed to define accessible names, values and segment bounds.
[Frozen root](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/time_field.rs#L532-L572)

Proposed portable contract

- Append time input and spinbutton roles. The ordered TimeField root owns
  logical focus and its formatted committed value. Its segments carry their
  localized committed text, discrete range, current numeric value and
  selected segment. The ordinary field retains its roles and routing.
- Carry optional semantic text separately from the label, description and
  transient input. Copy it on submission into the bounded frame text
  storage. A lazily allocated index, charged to the context resource
  budget, tracks text alongside the semantic snapshot without changing
  public structs. It is absent until a component uses it. Accessors exclude
  password and secret nodes, failed frames and nonfinalized data.
- AccessKit publishes the root and segment values, retaining their strings
  under its existing snapshot bounds. It keeps focus on the root time-input
  node. Browser proxies can expose a named spinbutton for the active
  segment, with numeric bounds, the localized committed value and the
  formatted field time as context. That accessible metadata leaves the
  proxy's native input buffer free for composition. The browser uses a
  group for the time-input root and hides the mirrored active segment while
  its equivalent proxy is present. [Spinbutton guidance](https://www.w3.org/WAI/ARIA/apg/patterns/spinbutton/)
- Retain existing focus dispatch. Segment display, keyboard selection,
  platform focus and support for accessibility value editing are distinct.
  An adapter may expose the current value without advertising new actions.
  An accessibility request to select or edit a passive child needs its own
  admission and ordered-dispatch tests.

Appending the formatted time to the name would change a control's stable
label. Carrying it in the description would conflate the value with help
text. Storing it in the DOM input buffer would change the browser's typing,
selection and duplicate-commit behavior. Enlarging the semantic structs
would change the layout of public arrays and by-value calls. A semantic
value accessor preserves that layout, allows adapters to compare values
independently and keeps memory bounded by the existing context budgets.

Verify portable source and distribution snapshots, value changes independent
of selection, UTF-8, hidden and secret nodes, source lifetimes and overflow.
Verify AccessKit publication and stable focus, then real Chromium AX tree
values and focus alongside keyboard and IME regression tests. Actual
screen-reader behavior still requires physical-host checks.

The portable roles and text accessor now back both adapters. A private Linux
AT-SPI client reads the localized formatted time through the platform's
description fallback and reads the segment's numeric range. Chromium's AX
tree reports the focused proxy as a spinbutton with its segment name,
numeric value and formatted field time while composing. Chromium derives
the editable node's AX value text from its native input buffer, even when
the host supplies an explicit `aria-valuetext`. The authored attribute still
carries the localized segment token, and the numeric value and full-time
description continue to describe committed state during preedit.

## Delegated segment focus checkpoint

The desktop accessibility tree can identify a TimeField segment, but the
segment's public focus capability is currently false. A segment cannot
become Core focus without changing Tab order and leaving the ordered
reducer. Programmatic focus on the whole field resumes the prior segment.
A pointer event would change selection but would misrepresent the input
producer and could activate an overlapping control.

Extend the existing ordered participant with an optional internal segment
selection operation. Keep the application and host APIs for focus. In a
successfully completed snapshot, a passive spinbutton can offer Focus if it
delegates to its own visible, enabled, published ordered time-input root
in the current modal scope. This capability does not add a Tab stop or
change direct `cui_focus`, generic focus proxies or ordinary TimeField.

On dispatch, the current admitted participant must recognize the requested
segment under its current options. Move logical focus to the root and
select the segment before routing subsequent input. Preserve the original
segment as the accessibility event target. A request for the already
selected segment keeps the current edit and composition. A change of
segment clears the draft and invalidates any qualified event for the old
IME session. Read-only fields accept Focus and preserve their values.
Focus neither supplies a value nor emits activation or a change callback.

Revalidate against submitted semantics, scope and published roots at frame
end. Withdraw the receipt for a target omitted by late layout. An omitted
root discards its staged edit and cannot retain focus. When the root is
still published, a late layout omission of one child leaves the already
routed logical focus on the root. Admission or publication cannot rewind
later events. Native publication projects the active root's focus onto its
selected eligible child. The gallery forwards DOM focus on its mirror as
a real accessibility request and transfers DOM focus to the active text
proxy using the ordinary IME handoff.

Platform value actions remain independent. Advertising Focus does not
advertise Activate, SetValue, Increment or Decrement. This extends an
existing ordered dispatch case without a new public dispatch mechanism.
Verify same-batch physical keys, selection transitions and stale IME, both
Tab directions, disabled/read-only/omitted cases, the Linux AT-SPI action,
and the browser's accessibility projection and ordinary input behavior.
