# TimeField applied CHANGE checkpoint

Design only, audited at `defb6f72`. The APIs below are proposed, not implemented.
Keep all 78 manifest entries and existing functions/options/state layouts unchanged.
This is a C observer slice, not a binding migration or full family parity claim.

## Existing operation and ownership

[`cui_picker.h`](../../include/cui/cui_picker.h) defines caller-owned `cui_time`
with unsigned hour/minute/second (24-hour model), `cui_time_state` with selected
segment/digit count/draft, and options seconds/twelve_hour/disabled/readonly.
There is no TimeField callback or acceptance policy. `cui_time_valid` admits
hour <24, minute/second <60; parse/format are separate programmatic utilities.
Hiding seconds preserves the model's second; twelve-hour display does not change
the model into a twelve-hour value.

[`cui_base_time_field`](../../src/base/cui_picker.c) validates required pointers
and initial time before doing work. It repairs unsupported selected segments and
digit_count >1 even with disabled/readonly options. Then it scans existing routed
input in order. Pointer-down selects a segment/reset draft; Left/Right moves focus;
Up/Down/Home/End changes a segment; AM/PM toggles half-day. Inline TEXT digits can
apply several owner writes inside one event, including draft validation and
focus advance after two digits. Invalid numeric drafts need not change the owner.
The current implementation reads `event.text`, not a new text provider/IME editor.
It does not implement composition, submit, paste transactions, undo or an edit
engine; this slice must not imply those features or change text handling.

Reduction precedes `cui_interact` for compound GROUP and segment TEXTBOX semantics,
readonly/range details and selected-segment tab stop. It finally compares all
three owner fields with entry, invalidates only on net change, and returns that
bool. It does not consume/replay inputs. Focus/draft repairs and hidden-segment
normalization are not time changes. Disabled/readonly skip the input loop while
retaining state repair/semantics work; disabled interaction may clear focus.

[`cui_time_field`](../../src/components/cui_picker.c) checks theme/state/time and
validity, calls Base exactly once, paints the compound/segments/separators from
final time/focus, then returns the Base bool. No late paint operation edits time.
No existing user callback runs in either path. Existing TimeField tests are
[`time_events`/`time_scene`](../../tests/unit/test_picker.c), compiled for both
Styled `picker` and `picker_base_only`: rollover, minute typing/advance, seconds,
AM/PM, twelve-hour Home/End, semantic ranges, readonly and disabled focus cleanup.
They establish existing behavior, not a typed observer contract.

## Proposed minimal API

Append to existing `cui_picker_events.h`, using its portable public C11 types:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_time previous, value;
} cui_time_change;
typedef void (*cui_time_change_fn)(void *, const cui_time_change *);
typedef struct { cui_time_change_fn on_change; void *user; } cui_time_handler;

cui_picker_event_result cui_base_time_field_events(
    cui_context *, cui_id, cui_str name, cui_time_state *, cui_time *,
    cui_time_options, cui_time_handler);
cui_picker_event_result cui_time_field_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, cui_time_state *,
    cui_time *, cui_time_options, cui_time_handler);
```

Reuse `cui_picker_event_result {bool changed; size_t change_count;}` without
changing its layout. Required pointers and nonzero compound ID are checked before
legacy work; Styled also requires theme. Invalid admission returns zero without
mutation. Otherwise snapshot the three entry fields and eligible active frame,
call the complete corresponding legacy operation **once**, then compare each
hour/minute/second explicitly (no padding/memcmp). Preserve its bool verbatim.
Missing observer ignores user, preserves all old work, and returns count zero.
A real final difference with observer and eligible frame yields exactly one CHANGE
and count one, even if the old bool someday differs; do not gate on that bool.

Base callback is after final semantic submission/invalidation, before caller's
subsequent layout work. Styled callback is after complete original paint. Never
call an observed Base inside observed Styled: that would notify twice/too early.
Implement as whole-operation wrappers in existing Base/Styled picker-events files;
no existing reducer/renderer changes, no allocations or caller workspace.

This intentionally matches Calendar/DatePicker/ColorPicker aggregate facades in
[`cui_picker_events.h`](../../include/cui/cui_picker_events.h), rather than
claiming per-input multiplicity. Multiple edits yielding one final difference
produce one callback; Up then Down returning to entry produces none. Multiple
digit writes inside one TEXT event similarly report final time only. There is no
REQUEST, rejectable owner transfer, COMMIT, SUBMIT, or draft/focus lifecycle event.

A per-transition alternative is feasible but materially different: factor every
owner write in the key/text branches into a narrow helper, snapshot each actual
write, and notify immediately before final semantics/paint. It could report net-zero
intermediate times and multiple digit writes, and preserve genuine input index/type.
It would expose partially updated draft/focus state and a different callback timing
contract. It is not needed for this bounded aggregate value-observer slice; do not
silently substitute it or claim aggregate count is transition/input count. It can
be reviewed independently if the user needs every intermediate transition.

## Evidence, lifetimes and suppression

Metadata kind CHANGE, target compound ID, source UNKNOWN. Include HAS_FRAME only
with frozen nonzero `cui_frame_sequence` from an active paint-enabled submission
and enabled/nonreadonly options. No event index/source/activation: one output can
combine routed segment events and several writes. The wrapper cannot identify one
causal activation by scanning after reduction. Existing reduction precedes semantic
registration and accepts raw key/text targeting rather than Button activation;
never register early or fabricate/drop a proven per-input snapshot for convenience.
This aggregate API makes no per-input evidence claim.

[`cui_frame_sequence`/`cui_event_target`](../../src/core/cui.c) suppress inert and
layout-only routing/evidence. Freeze eligibility before the old operation; zero
frame, passive/layout-only/inert, disabled or readonly means no new observation,
while old state repair, focus policy, semantics and paint still run unchanged.
External changes before entry are the baseline, not programmatic notifications.
Parse/format are unchanged; no new helper manufactures frame facts.

Payload and time fields are pointer-free/copyable beyond callback/frame/context
lifetime. Callback/user/payload pointer are borrowed synchronously. Caller must
keep context, time/state, label/theme and associated frame resources live through
outer return and renderer consumption. Observer must not mutate/rebind/free active
time/state/context/resources, input, or recursively submit UI. Unrelated flags
and copying payload are allowed; defer omission/destruction until after that
boundary. No buffer binding, retained workspace, hidden owner or thread dispatch
is added. The observer cannot reject an already applied change.

## Verification and bounded implementation ownership

Proposed files only: `include/cui/cui_picker_events.h`, existing
`src/base/cui_picker_events.c` and `src/components/cui_picker_events.c`, new
`tests/unit/test_time_field_events.c`, new `docs/TIME_FIELD_EVENTS.md`, and this
checkpoint status. No Core/legacy Picker reducer/renderer, root build wiring,
SDK/generated files or other families. Root registers the new test/export later.

Tests must compare old/new Base and Styled owner/state/result/focus/invalidation,
semantics (values/ranges/readonly/tab stops) and Styled draw commands/text/bounds:

- Hour/minute/second rollovers, Home/End, twelve-hour AM/PM, hidden seconds retained.
- Multiple routed keys and digit TEXT writes, one callback for net change; net-zero
  key edits silent; invalid draft/focus/selection-only movement silent.
- Pointer segment targeting followed by editing, and existing unsupported a11y/IME
  boundaries documented rather than inventing activation or composition support.
- Callback sees final owner/semantics and already-submitted Styled paint; exact
  UNKNOWN/frame-only flags, copied payload survives teardown and deferred omission.
- Missing observer still edits; invalid/null admission and invalid time atomic;
  disabled/readonly state maintenance and focus policy equivalent; passive/inert
  notifications suppressed without suppressing legacy work.

Strict C11 warnings, C++ header inclusion, and private `scripts/test_local.py`
wrapper for new test plus `picker`, `picker_base_only`, `picker_events` regressions.
No builds or production changes are authorized by this checkpoint itself.

## Existing foreign surface and remaining gap

[`src/bridge/cui_bridge.c`](../../src/bridge/cui_bridge.c) already renders legacy
TimeField with seconds=true and disabled flag; a true old bool enqueues generic
`bridge_change`. [`test_time_field`](../../bindings/bridge/tests/test_bridge_components.c)
checks time setter/getter validation, keyboard mutation, CHANGE serialized time
text and disabled silence. That queue is not this proposed synchronous typed C
callback, and does not contain typed previous/value time pairs.

Python `time_value/current_time/on_change`, Node equivalents and TypeScript
`TimeField` declarations, Go `Time/SetTime`, `App.Time` and `OnChange`, Rust
`time/current_time/on_change_event` provide existing value and generic event paths
in their canonical binding sources. Their tests include Python/Node model tests,
Go `TestTimeField` and Rust builder time validation. This slice neither removes
those paths nor claims new typed callback bindings, previous-time serialization,
readonly/twelve-hour option exposure, or identical callback timing across foreign
runtimes. A later bridge/binding slice needs an explicit payload/lifetime decision.

## Implementation checkpoint

The proposed Base/Styled aggregate wrappers, typed payload/handler and guide are
implemented without changing the legacy reducer/renderer. Tests compare legacy
and facade behavior, including actual inert/layout-only scopes and invalid hidden
seconds; no additional metadata, IME or binding support is claimed. Root build/test
registration and generated exports remain integrator-owned.
