# Single-date DatePicker time editing: design checkpoint

Design only; no new declarations or behavior are implemented by this checkpoint.
Worker base: `0849cf6eeed95bdbc084ca0c9360699deec087ca` (next59 gallery regressions).
Frozen reference: `87d10ae5e1299d1be18670a37570c5402f952e34` at
`/tmp/cui-gpui-reference`. Legacy date-only APIs and layouts remain unchanged.

## Source and current evidence

Frozen `crates/component/src/time/date_picker.rs`:

- 215–227: Calendar Selected calls `select_date`; TimeField Change updates retained
  start time and calls `emit_change`.
- 261–291: minute/second precision truncates times; hour-cycle configuration marks
  the field for synchronization; changing default time resets current times too.
- 327–343: `set_date_time` sets supplied times (empty uses default), pushes the
  child time, then updates date without Change; the update closes the popup.
- 359–399: only single values edit time. Child configuration is pushed on explicit
  picker changes, not every render, to avoid overwriting undelivered field edits.
  Change is emitted only for complete values by `emit_change`.
- 403–420: a different accepted day changes date and emits Change while staying
  open; the same accepted day closes without another Change. This comparison is
  with the OUTER picker date, not the possibly divergent retained child date.
- 423–447: programmatic/preset update stores outer date before child Calendar
  acceptance, closes directly; a rejected child may retain its old selection.
- 484–499: clear restores default time, clears date, closes and emits Change,
  without an explicit trigger-focus request.
- 508–526: date-only presets preserve current time; date-time presets replace
  and truncate time. Both close, emit Change and unconditionally focus trigger.
- 529–538,646–670,844–854: default display adds precision/hour-cycle formatting;
  popup time row uses a localized label and TimeField. Range values can store times
  upstream but do not edit both endpoint times in the popup.

Transitive authority remains `crates/base/src/calendar.rs:118–124,217–235`:
matcher rejection retains child state; accepted nonempty assignment recenters
month/year without resetting view/cursors. Actual day selection must adopt the
child even when its value is unchanged but differs from outer ownership.

Current C contracts are `include/cui/cui_date_picker_actions.h`,
`docs/DATE_PICKER_ACTIONS.md`, `include/cui/cui_date_picker_calendar.h`, and
`include/cui/cui_picker.h:105–123`. Current action state already retains separate
outer observation and child selection. Its Base implementation invokes
`cui_picker_actions_reduce`; `src/base/cui_picker.c` then automatically closes
on complete Calendar activation. Calling this facade and reopening afterward is
not acceptable: shared close can already restore focus/withdraw semantics.

Current TimeField retains `segment`, `digit_count` and `draft`; its reducer
(`src/base/cui_picker.c:1041–1160`) updates valid numeric prefixes, advances after
second digits, clamps/wraps via keys, and submits segment semantics. It does not
own a date or distinguish an external assignment from repeated controlled values.
The wrapper must own that distinction. No general text parser, timezone, locale
service or clock is required.

## Architectural decision proposed for review

Add a separate `cui_date_time_picker.h` with explicit state and single-date value.
Do not append time fields to legacy selections/events or change date-only presets.
Reuse the bounded action identity machinery and Calendar/TimeField primitives
through private composition helpers, not a second public-facade reduction.

A narrow private Calendar composition seam must return its activation/change
result BEFORE choosing close/focus policy. Existing callers retain auto-close;
new datetime composition compares accepted child date with outer date, then
chooses keep-open or same-day confirmation. If this cannot be implemented without
changing shared Core/Surface dispatch, stop for architecture review.

Proposed exact public declarations (names/layout subject to review):

```c
#include "cui_date_picker_actions.h"

typedef enum {
    CUI_DATE_TIME_MINUTE = 0,
    CUI_DATE_TIME_SECOND
} cui_date_time_precision;

typedef struct {
    cui_date date; /* All-zero means absent; otherwise a valid Gregorian date. */
    cui_time time; /* Valid even when date absent: explicit retained time of day. */
} cui_date_time_value;

typedef struct {
    cui_id id;
    cui_str label;
    cui_date date; /* Presets require a present valid single date. */
    bool has_time;
    cui_time time; /* Valid when has_time; ignored otherwise. */
} cui_date_time_preset;

typedef struct {
    cui_date_picker_options picker;
    cui_date_picker_calendar_options calendar;
    cui_date_time_precision precision;
    bool twelve_hour;
    cui_time default_time;
    cui_str time_label; /* Empty uses "Time"; borrowed UTF-8. */
    const cui_date_time_preset *presets;
    size_t preset_count;
    cui_id clear_id;
    cui_str clear_label;
    bool cleanable;
    cui_date_picker_delete_policy delete_policy;
    cui_date_picker_preset_policy preset_policy;
} cui_date_time_picker_options;

typedef struct {
    cui_date_picker_actions_state actions; /* Internal ownership/identities. */
    cui_time_state time_field;
    cui_date_time_value observed;
    cui_time applied_default;
    cui_date_time_precision applied_precision;
    bool applied_twelve_hour, configured, assignment_pending;
    /* Copyable state, not user-editable bookkeeping. No pointers retained. */
} cui_date_time_picker_state;

typedef struct {
    cui_calendar_result calendar;
    bool valid, value_changed, source_change;
    bool time_edited, action_activated, confirmed;
} cui_date_time_picker_result;

void cui_date_time_picker_init(cui_date_time_picker_state *, cui_id popup,
                               cui_date initial_month);
/* Between frames: checked assignment marks even an identical value for sync. */
bool cui_date_time_picker_assign(cui_date_time_picker_state *,
                                 cui_date_time_value *controlled,
                                 cui_date_time_value next);
cui_id cui_date_time_picker_time_id(cui_id trigger);
cui_date_time_picker_result cui_base_date_time_picker(
    cui_context *, cui_id trigger, cui_id calendar, cui_id time,
    cui_str name, cui_date_time_picker_state *, cui_date_time_value *,
    cui_date_time_picker_options);
cui_date_time_picker_result cui_date_time_picker(
    cui_context *, cui_id trigger, const cui_theme *, cui_str name,
    cui_date_time_picker_state *, cui_date_time_value *,
    cui_date_time_picker_options);
/* After Base reduction, inside caller's one open popup scope. Semantics only. */
void cui_date_time_picker_submit_presets(
    cui_context *, cui_date_time_picker_state *, cui_date_time_picker_options);
```

The explicit time while date is absent is a C representation difference: upstream
`DateTime::Single(None)` hides retained start time. It permits deterministic
controlled ownership without a hidden second writable time value. Consumers
must test date presence before treating this pair as a committed date-time.
`value_changed` compares both fields; an empty-date time edit can therefore change
storage without `source_change`. This is not a new source Change emission.

MINUTE truncates seconds to zero; SECOND retains seconds. No timezone, UTC offset,
DST interpretation, epoch conversion, leap seconds or implicit current time.
`cui_time` stays a 00–23 hour civil time even in twelve-hour presentation.

## Synchronization, reduction and result contract

All options/value/IDs preflight before edits, predicates or event consumption.
Reject malformed/partial dates, invalid active time, unknown enums, non-single
`picker.calendar.range`, invalid bounds/viewport, duplicate/reserved IDs, capacity
or active UTF-8 errors. Reuse action limits (8 presets,96-byte labels), exact ID
collision checks and whole-list validation. SOURCE permits outer preset assignment
when child matcher rejects; ENABLED rejects in advance using the existing explicit
policy including bounds. Shape policy is vacuous because this facade is single.

First valid reduction configures children, using supplied controlled value as the
initial outer value; default time does not overwrite a present supplied value.
An absent initial value starts with the truncated configured default. Record the
normalized observed value after all accepted changes, preventing self-feedback.

On later frames, a changed external value or explicit `assign` is source setter
intent: reset time draft, truncate supplied time (empty assignment uses default),
sync Calendar with retained-child rejection semantics, close directly without an
explicit focus request, and emit no `source_change`. An equal ordinary value is
NOT a setter: preserve draft, view and divergent child. Explicit assign validates
all next value fields atomically, writes controlled storage and marks pending;
NULL/invalid inputs do nothing. It only queues child/UI synchronization for the
next valid frame. It does not consume events or require a context between frames.

Configuration changes apply once, before input: default-time change resets current
time (matching its source builder); precision change truncates time/default;
hour-cycle change resets draft but retains civil time. On simultaneous changes:
configure first, external setter second (external value wins), queued UI input
last. No source Change is invented for configuration alone. Changing matcher or
reopening does not implicitly resynchronize a previously rejected child.

Reducer phase order is explicit, preserving CUI's phase-based action behavior:
external/config sync; Escape/overlay/trigger pass; one Calendar reduction; one
TimeField reduction if still open; one ordered clear/preset/Delete pass. It is not
a promise of global chronological interleaving across Calendar and TimeField.
Calendar selection compares against outer date BEFORE adoption:

- Different accepted day: adopt child date, preserve current time, remain open,
  `source_change=true`; no trigger focus request.
- Same accepted day: close through ordinary conditional focus restoration,
  `confirmed=true`, no date Change. A no-op child activation with a divergent outer
  date follows the different-day rule. Disabled/rejected days cannot confirm.
- Time edit with a present date: preserve date, stay open, source Change on actual
  TimeField change. Empty-date edit updates retained time but emits no source Change.
- Clear/Delete: restore truncated default, empty date/child, reset draft/range phase,
  preserve Calendar view/month, direct close without explicit focus, source Change
  even if already empty. Presets: date-only preserves time, timed replaces/truncates;
  sync child, close, source Change even if identical, unconditional trigger focus.

Outer disabled gates all user actions; Calendar-only disabled gates Calendar input,
not clear/presets or time editing. No additional readonly policy in this slice.
Ordinary close/Escape retains draft; reopening alone does not push time again.
While time UI is omitted, no TimeField reducer runs and stale targets cannot edit.
Published identity records expire on omission, failed helper, or close/reopen as in
existing actions. Scope Delete includes actually submitted eligible time segments
as well as Calendar/preset targets. Popup root and unsubmitted namespace IDs do
not suffice. Core may resolve invalid programmatic focus to a real modal target;
that resolved event target remains authoritative.

Approved Base correction: Base reduces input once but submits no TimeField
bindings before the caller opens its popup scope. The private TimeField seam
splits reduce and submit, while the legacy public TimeField still does both.
After Base reduction, inside the one actual popup scope: submit Calendar, call
`cui_date_time_picker_submit_time(ui, time_id, state, value, options)`, THEN call
`cui_date_time_picker_submit_presets`. The latter snapshots actual eligible
Calendar/preset/time targets. Both helpers are semantics-only; neither reduces,
starts a scope, invokes predicates through input reduction, or replays callbacks.
Call the preset helper even for zero presets to publish the bounded admission set.
Failure or omission of either helper prevents stale time admission next frame.
Caller paints those segment IDs, Calendar and helper rows. A later Styled facade
must use paint-only seams; calling the public TimeField again would reduce twice
and is prohibited. Slice1 has no unimplemented Styled declaration. Define/verify derived root and segment IDs against clear/presets and
Calendar before reduction. The previous frame must provide genuine bindings;
retaining numeric IDs cannot synthesize activation.

`source_change` is a coalesced per-submission fact that one or more source Change
conditions occurred, not an ordered callback stream. `value_changed` is final vs
entry storage; `confirmed` is same-day acceptance. No typed observer is included;
future events must separately specify order, repeated equal notifications and
empty-vs-complete payload. Late Core/layout failures are not transaction rollback;
`valid` describes preflight only. Invalid options never partially normalize value.

## Lifetimes, formatting and limits

State and controlled storage belong to one owner UI thread and context. Reinit
before moving context; init(NULL) is a no-op. State contains copied values only;
no callbacks/OS resources/timers are retained. Pointers must be readable for calls;
labels/locale/predicate resources remain valid through frame/draw consumption.
Predicates remain pure and must not reenter UI. No allocation/new dependency.

Default display combines existing date locale formatting and bounded time format:
minute/second and explicit 12/24-hour appearance. Offer a checked bounded format
helper during implementation if necessary; do not implement chrono patterns or
parsing of arbitrary human strings. Time-label translation is explicit. General
style, responsive scrolling, full lifecycle observers, range-time storage/editing,
OS localization and live AT remain excluded. Existing modal outside-clear rule
continues: dismiss before activating clear outside an open popup.

## Independently reviewable production slices and acceptance

1. Portable value/config/sync + Base composition. Review exact public layout and
   private suppress-auto-close/paint-only seams first. Preserve old state/layout
   and zero-option source behavior. Native/WASI tests: first/external/equal/forced
   assign; drafts across idle/reopen vs reset; 12/24 hour and precision; leap-day/
   midnight fields; different-day keep-open and same-day close; divergent retained
   child no-op adoption; bounds/matcher changes; invalid preflight; disabled and
   consumed/stale events; mixed Calendar/time/action phase order; empty-time
   storage vs source-change distinction; repeated clear/preset; identity expiry.
2. Styled popup and presets/clear integration, bounded formatter and distribution.
   Actual semantic targets and software paint assertions across sizes/themes,
   full popup fit/failure, segment focus/Tab, clear focus vs preset focus, and
   legacy pixel/memberwise semantic equivalence. C++ include, strict C11 and
   ASan/UBSan through `scripts/test_local.py`; same deterministic WASI vectors.
3. Explicit opt-in WASM gallery mode preserving date-only defaults and copyable
   compiling snippet. Real headless desktop keys/pointer and CDP touch: day then
   time editing, same-day confirm, draft preservation, clear/preset and disabled
   paths, narrow bounds and Escape. No desktop launch or calendar-clock oracle.

Bindings/builders and typed picker observers do not automatically gain this API.
Integrator must explicitly register public headers/sources, distribution, metadata,
consumer guidance and any later binding builders. Do not promote full DatePicker
parity: existing date-time range/source formatting/style/event boundaries remain.

No tests were executed for this documentation-only checkpoint. Source inspection
is evidence for the proposed contract, not proof that it is implemented.


Production slice1 clarification: `time_id` and `time_submitted_frame` are copied
bookkeeping in the new state. A successful time submission precedes publication
of the aggregate target set. Final value changes and coalesced source Change are
separate; regression vectors include time edit followed by timed preset restoring
the entry value. No production Core/Styled/canonical registration change belongs
to the Base-only slice.

## Review checkpoint: ordered accepted-day decisions (implementation paused)

The phase-final Calendar result is insufficient for date-time confirmation. With
outer date A, accepted B followed by accepted A must adopt B, then A, emit two
source Change conditions, and remain open. Comparing the final Calendar value
with the entry value would instead close incorrectly. Production slice1 is not
ready to ship until this is corrected; its existing single-acceptance tests do
not establish this multi-event behavior.

Authority is frozen `87d10ae5e1299d1be18670a37570c5402f952e34`:
`crates/base/src/calendar.rs:260-269` commits the Calendar selection before
emitting Selected; `crates/component/src/time/date_picker.rs:403-420` compares
that selected value against the picker's *current* date, closes and returns on
equality, or stores the new date and calls emit_change on difference.
`emit_change:395-401` emits only for a complete date. This establishes the order
of individual source handlers; it does not establish equivalence between GPUI's
queued event scheduler and CUI's frame input queue.

### Smallest proposed private seam (not yet approved or implemented)

Add these private types to `src/base/cui_picker_composition_internal.h`; no public
Calendar state, result, event schema or Core dispatch changes:

```c
typedef enum {
    CUI_PICKER_ACCEPT_CONTINUE,
    CUI_PICKER_ACCEPT_STOP
} cui_picker_accept_flow;
typedef cui_picker_accept_flow (*cui_picker_accept_visitor)(
    void *context, size_t event_index, cui_date_selection accepted);
```

Extend only the private Calendar reducer implementation with visitor/context
arguments. All existing public Calendar and legacy picker entry points pass
NULL/NULL. The private datetime reducer supplies one stack-owned accumulator.
No visitor, context pointer or event index survives the synchronous reduction.
This is a private internal function pointer, never an application callback.

Invoke the visitor exactly once at the single successful activation site in
`src/base/cui_picker.c` (currently immediately after `result.activated = true`),
after candidate validation/selection commit and existing recenter/focus updates.
Pass a copied selection and the current Core event index. Navigation alone and
rejected candidates do not invoke it. Existing range reducers use NULL; datetime
preflight prohibits range. This preserves existing CUI matcher/bounds admission;
it does not newly claim exact source notification behavior for rejected dates.

The datetime visitor compares each accepted single date with its accumulator's
current outer date, initialized after external assignment/configuration sync:

1. Different: adopt that date immediately in the accumulator, retain current
   time, set coalesced source_change, and return CONTINUE.
2. Equal: set confirmed and return STOP without setting source_change for this
   acceptance. Retain any earlier source_change already observed.

STOP breaks only this Calendar reduction's remaining event loop. Existing final
cursor normalization, net Calendar changed calculation and invalidation still
run. After the reducer returns, datetime copies the accumulator's final date to
its controlled value and performs overlay close/focus handling once. The visitor
must not call CUI, invoke predicates, dispatch input, mutate Calendar state,
allocate, or call any user callback. Time reduction is skipped after confirmation,
as it already is for a closed popup. This avoids reentrant overlay mutation
inside Calendar reduction.

Stopping at the first confirmation is an explicit CUI popup-generation routing
policy: later queued Calendar events cannot mutate a popup that has confirmed.
It is not a claim about GPUI scheduling after an emitted close. Thus A→B→A
remains open; A→B→B confirms B and ignores subsequent Calendar events; A→A→B
confirms A and ignores B. Public source_change remains a coalesced fact and
value_changed remains final-versus-entry; neither is used to decide confirmation.
Source callbacks/event-stream parity remains excluded.

### Capacity, failure and compatibility

The seam uses constant stack storage and no trace array, so there is no trace
capacity, allocation failure or truncation policy. At most one visit occurs per
input event that the existing bounded Core frame admitted, and processing stops
on confirmation. The seam adds no independent queue or event retention. An
invalid enum returned by this internal visitor is treated as STOP defensively;
no partially published trace or callback state exists. Preflight failures still
happen before the reducer and do not invoke the visitor. Core frame/layout
failure remains the existing nontransactional failure boundary, not rollback.

NULL visitor preserves the exact existing loop and legacy result semantics,
including processing later events and computing changed from final versus entry.
No public state layout changes; the new date-time state needs no additional
retained fields. There must be no second Calendar reduction or event replay.
This seam addresses accepted Calendar ordering only. TimeField currently reports
net phase change; its Up→Down cancellation/event-notification fidelity must be
reported separately rather than inferred from this Calendar correction.

### Required focused proof before production handoff

Use actual prior-frame semantic day targets and queued events, not direct
visitor calls alone. Cover A→B→A (open, source_change true, value_changed false),
A→B→C (open, final C), A→B→B (closed B, changed and confirmed), A→A→B
(closed A, no source_change), and divergent outer/retained child followed by
multiple accepted days. Include rejected/disabled candidates between accepted
ones, navigation preceding acceptance, matcher rejection with no visitor, and
subsequent TimeField input suppressed after confirmation. Record legacy
Calendar/date-only picker results for equivalent queues with NULL visitor.
Run native wrapper, strict C11/C++ header checks, sanitizer and deterministic
WASI vectors. No claim of corrected behavior until those tests pass.
