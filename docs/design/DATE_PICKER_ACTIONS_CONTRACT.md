# DatePicker date-only actions: proposed contract

Reviewed contract, based on `b0405a3c`; implementation is isolated in the new
actions files and private fixture. Canonical registration remains integrator-owned.
Final validation evidence is recorded in the consumer guide.
Scope: presets and clear actions, legacy-preserving Base/Styled composition;
no date-time, new visual variants, lifecycle observers or Core dispatch changes.

## Proposed exact declarations

New `include/cui/cui_date_picker_actions.h` includes
`cui_date_picker_calendar.h`. All declarations use portable C11 existing types.

```c
#define CUI_DATE_PICKER_PRESET_LIMIT 8u
#define CUI_DATE_PICKER_LABEL_LIMIT 96u
#define CUI_DATE_PICKER_TARGET_LIMIT 144u

typedef enum { CUI_DATE_PICKER_SINGLE = 0, CUI_DATE_PICKER_RANGE } cui_date_picker_shape;

typedef struct {
    cui_date_selection dates;
    cui_date_picker_shape shape;
} cui_date_picker_value;

typedef struct {
    cui_id id;
    cui_str label;
    cui_date_picker_value value;
} cui_date_picker_preset;

typedef enum {
    CUI_DATE_PICKER_PRESET_SOURCE = 0,
    CUI_DATE_PICKER_PRESET_MATCH_SHAPE,
    CUI_DATE_PICKER_PRESET_MATCH_SHAPE_ENABLED
} cui_date_picker_preset_policy;

typedef enum {
    CUI_DATE_PICKER_DELETE_OFF = 0,
    CUI_DATE_PICKER_DELETE_TRIGGER,
    CUI_DATE_PICKER_DELETE_SCOPE
} cui_date_picker_delete_policy;

typedef struct {
    cui_date_picker_options picker;
    cui_date_picker_calendar_options calendar;
    const cui_date_picker_preset *presets;
    size_t preset_count;
    cui_id clear_id;
    cui_str clear_label;
    bool cleanable;
    cui_date_picker_delete_policy delete_policy;
    cui_date_picker_preset_policy preset_policy;
} cui_date_picker_actions_options;

/* Separate retained Calendar selection is required for source policy. */
typedef struct {
    cui_date_picker_state picker;
    cui_date_picker_value calendar_value;
    cui_date_picker_value observed_value;
    bool synchronized;
    /* Previous submission identities, copied values only; do not edit directly. */
    cui_calendar_state submitted_calendar;
    cui_id submitted_root, submitted_presets[CUI_DATE_PICKER_PRESET_LIMIT];
    size_t submitted_count;
    bool submitted_open, submitted_clear;
    uint64_t prepared_frame, submitted_frame, clear_frame;
    cui_id prepared_root, prepared_trigger, submitted_clear_id;
    unsigned submitted_months, submitted_first_weekday;
    cui_id submitted_targets[CUI_DATE_PICKER_TARGET_LIMIT];
    size_t submitted_target_count;
} cui_date_picker_actions_state;

void cui_date_picker_actions_init(cui_date_picker_actions_state *, cui_id popup,
                                  cui_date initial_month);

typedef struct {
    cui_calendar_result calendar;
    bool valid;
    bool changed;
    bool action_activated;
} cui_date_picker_actions_result;

/* Inside caller-owned open popup scope, after a valid Base reduction.
 * Publishes bounded prior-submission identity only after full successful submission.
 * Semantics only: no reduction, callbacks or scope begin/end. */
void cui_date_picker_actions_submit_presets(cui_context *, cui_date_picker_actions_state *,
                                            cui_date_picker_actions_options);

cui_date_picker_actions_result cui_base_date_picker_actions(cui_context *, cui_id trigger,
                                                            cui_id calendar, cui_str name,
                                                            cui_date_picker_actions_state *,
                                                            cui_date_picker_value *,
                                                            cui_date_picker_actions_options);

cui_date_picker_actions_result cui_date_picker_actions(cui_context *, cui_id trigger,
                                                       const cui_theme *, cui_str name,
                                                       cui_date_picker_actions_state *,
                                                       cui_date_picker_value *,
                                                       cui_date_picker_actions_options);
```

The new state is necessary: frozen source owns independent picker and child
Calendar selections. `init` initializes `picker` with the existing init, sets
`calendar_value` to empty single and `synchronized=false`; the first successful
preflight synchronizes the supplied outer value under the policy below.
`observed_value` tracks the last processed outer value, not the child value.
An externally changed outer value is synchronized once, never retried implicitly
on every frame after child rejection. This prevents reopening/render from erasing
a retained child divergence. A later matcher change alone does not retry it.
The caller owns this state; no allocation or hidden retained pointer is introduced.

`value.shape` is authoritative for the picker title and clear action. Child
Calendar reduction/submission uses `state.calendar_value.shape` and dates;
`options.picker.calendar.range` is ignored by this facade. All other picker and
Calendar options are forwarded. An accepted complete child activation publishes its child value to the outer
value even if the child date did not change (frozen Calendar emits Selected).
Actual child edits additionally publish incomplete ranges, preserving CUI's
partial-range behavior; upstream subscription delivery is completion-only.
Navigation and rejected activation do not copy stale child values over the outer
value. Legacy records/functions
remain unchanged. Zero-action equivalence to legacy holds only when outer and
child selections are synchronized; exact source divergence is new explicit state,
not a claimed shared-selection legacy behavior.

## Defaults and eligibility

Zero options have no presets, clear affordance or Delete action. The old functions
are unchanged; synchronized zero-action calls preserve the corresponding `_ex`
path with explicit shape. Divergence is documented separately above.
Zero preset policy matches frozen source's shape-changing behavior: a valid
single preset can replace a range and vice versa. Source policy accepts the outer picker value independently of child Calendar
eligibility; it does not bypass basic validity or bounded preflight. The child
sync then validates present endpoints using the live date predicate/bounds. A
rejected child update leaves its selection, month/year, view and range phase
unchanged while outer value/title changes and popup closes. An accepted nonempty
child update replaces child selection, sets phase from endpoint shape and recenters
month/year to its start; view and year-page state remain unchanged. This is the
source-recentering path, independent of Calendar day-activation recenter policies.
Using CUI min/max as well as its predicate is an explicit portable eligibility
extension; frozen source uses its matcher. No range interior scan occurs. `MATCH_SHAPE` rejects a mismatched preset
activation without mutation, but retains source-style eligibility bypass.
`MATCH_SHAPE_ENABLED` additionally requires each preset endpoint to satisfy
`cui_calendar_date_enabled` using live options. No interior range scan is implied.
These policies have distinct enum values, not an overloaded validation boolean.

Clear visibility and Delete are independent. `cleanable` shows a separate clear
button only if at least one endpoint is present. Delete defaults off;
`DELETE_TRIGGER` handles an unmodified Delete key whose resolved target is the
trigger; `DELETE_SCOPE` additionally accepts a target within the open popup scope.
Backspace, text input, modifiers and unrelated focused controls do not clear.
The DatePicker owns no text-entry child in this slice. Outer `picker.disabled`
gates trigger/preset/clear/Delete actions; Calendar-only disabled suppresses day
input, not these independent actions. This distinction is documented rather than
inferred from the child Calendar. Scope Delete must not intercept future editable
children; introducing a time editor requires revisiting this explicit contract.

An enabled clear empties the outer and child values while preserving outer
shape; empty values cannot be rejected by the date predicate. It resets child
`selecting_end`, preserves displayed month/year, view/cursors/year page, closes
and performs **no explicit focus request**, matching `clean` rather than
`set_open`. Existing overlay close helpers that restore focus must not be used
unconditionally for this action; a private no-focus close seam may be necessary.
Preset acceptance closes and **unconditionally requests trigger focus**, matching
`select_preset`, even if focus was elsewhere. Outside dismissal retains existing
conditional focus rules; these are separate paths, not one generic close policy.
A clear/preset equal to the current outer value counts as accepted action but
`changed` remains false. No observer emission semantics are introduced.

`MATCH_SHAPE` rejects a mismatched activation atomically; for matching shape it
retains source outer-accept/child-reject semantics. `MATCH_SHAPE_ENABLED` rejects
mismatch or ineligible child endpoints before either selection changes. Accepted
nonempty values in either policy use the same source-style child recenter/focus
behavior. Externally supplied values always use source-style child synchronization
once: an opt-in action rejection policy cannot reject a caller's assignment.

The alternative architecture is to keep the old shared selection and expose only
atomic enabled presets, with a name that does not imply source-equivalent policy.
That smaller design cannot offer SOURCE semantics. This checkpoint recommends
explicit separate child state if SOURCE remains required; integrator review is
needed before implementing that state distinction.

## Preflight, capacity, identity and lifetime

Before any focus/state/value changes or event consumption, validate all pointers,
nonzero distinct trigger/calendar/popup IDs, enums, value shape, options and all
preset records. `preset_count <= 8`; nonzero count requires an array. Each label
is nonempty valid UTF-8 of at most96 bytes, without embedded NUL. Active clear
support (cleanable or Delete policy not off) requires a nonzero distinct
`clear_id` and nonempty bounded UTF-8 clear label. An inactive clear record is
ignored, allowing exact zero defaults. Preset IDs must be nonzero, mutually
distinct and distinct from trigger/calendar/popup/active-clear IDs.

Known generated Calendar navigation/month/year/day/neighbor IDs for the current
submission must not collide with action IDs; reject a detected collision during
preflight. This is not a universal namespace proof: caller roots and action IDs
remain subject to ordinary global collision discipline. Do not claim hash IDs
are globally unique. Labels are borrowed through frame/render consumption;
array/value records through the call. No pointers, handlers or predicate results
are retained. Predicates remain pure and owner-thread only.

Dates are either all-zero absent or valid Gregorian1..9999 values. A single value
must have absent end; range permits empty, start-only or ordered complete dates,
but not end-only or reversed endpoints. Presets must be complete: one date for
single, ordered start/end for range. Basic invalidity in *any* preset rejects the
entire options preflight, even if not activated. Eligibility rejection is an
activation result, not invalid options. Preflight validates finite positive
viewport/width/cell geometry under explicitly documented normalization rules;
existing valid zero/default width/cell values retain legacy defaults. Calendar
min/max/date/options validation must match documented legacy accepted defaults;
new strictness must not silently label old valid options invalid.

On preflight failure return all-zero result, including `valid=false`, and make
no state/value/focus changes (including synchronization), consume no events, submit no control, and call no
date predicate. Normal successful preflight sets `valid=true`. Subsequent layout
allocation/overlay failure can still occur after reduction; `valid` is not a
transactional rollback promise. Document/test this separately from preflight.

## Event ordering and one reduction

Preserve the current outer pass order: Calendar-view Escape prepass, overlay
outside-dismiss/trigger toggles, then one entire Calendar reduction. Resolve its
original grid exactly as current `_ex` does; no second reduction/submission is
allowed. New actions form a final pass over the original frame event order.
Clear/Delete/preset targets are distinct from Calendar targets, so the earlier
Calendar pass does not consume them. This is explicitly **phase ordering**, not
global chronological interleaving between day and action events.

Action eligibility uses the resulting live shape and current disabled-date
predicate. Child and outer shape can differ under source policy; action shape
matching always compares with the outer value. Only actions targeted to the prior submitted clear/trigger/open popup
controls are accepted; closed popup stale preset IDs cannot reopen or edit.
Snapshot whether popup action controls were eligible at the start of this call;
current trigger toggling or Calendar auto-close cannot manufacture new submitted
action targets. The final action pass accepts at most one action: the first
eligible accepted clear/preset/Delete wins; consume only that event, then ignore
later action events for this call. Rejected mismatch/eligibility actions do not
win, mutate, close or consume. This prevents multiple presets after close and
makes clear versus preset deterministic. A day edit may precede the winning
action in the same call, consistent with preserved Calendar phase ordering.

`calendar` reports the actual Calendar reduction result. `changed` compares the
final value (including shape) with the value before this call; a later action
that restores the original value yields false. `action_activated` reports one
accepted action independently of net change. Do not pass these semantics through
the old typed aggregate event wrapper without a separate review.

Styled calls the common private reducer once, then paints trigger, separate clear
hit target and up to8 preset rows, and submits Calendar using the existing
presentation options. Base submits the same trigger/action semantics, with
caller-owned layout/popup painting as in the current Base contract. Both share
IDs, eligibility, queue/focus semantics and result. Styled uses bounded popup
geometry; viewport clipping must not make hidden preset rows actionable. No new
scroll service, style variants or custom renderer callback is included.

## Acceptance before production handoff

Add a standalone private fixture with Base/Styled runs covering zero-action
legacy equivalence, all three preset policies, shape changes, invalid-array last
record atomicity, bad UTF-8/count/IDs/geometry, no predicate call on invalid
preflight, disabled distinctions, empty and partial range clearing, modified
Delete rejection, trigger/scope/outside targeting, original day+action phase
ordering, first accepted action wins, stale closed-popup IDs, unconditional
preset focus, clear with no explicit focus request, outside-dismissal focus rules
and net-zero edits. Add retained child-rejection tests: outer title/value changes
while child selection/month/view stay fixed; reopening does not silently retry;
accepted child sync recenters month/year without resetting view/year page; clear
empties divergent selections without matcher invocation; actual subsequent day
edit resynchronizes values. Include software pixels and semantic names/
expanded/controls for the visible clear/preset controls; no live AT claim.
Compile C11/C++ header probes and strict warnings. Run fixture via private Xvfb
wrapper with meaningful sanitizers. Root owns canonical registration, bindings,
metadata, skill and generated distribution updates after reviewing this contract.

## Transitive source evidence correction

This revision supersedes the earlier incorrect “preset does not recenter” and
conditional preset-focus statements. Frozen paths inspected together:
`component/src/time/date_picker.rs:323–340,423–434,484–499,508–526` and
`base/src/calendar.rs:118–124,217–235`. Accepted child `apply_date` changes date
and current month/year only; rejected matcher updates change none of those.
`sync_children` only shares matcher/time configuration and does not resynchronize
child dates on each render. Clear emptiness bypasses matcher evaluation and
preserves month/year because there is no start. These claims are source evidence,
not executed upstream behavior or live focus/AT proof.


## Implemented split submission and identity expiry

`cui_date_picker_actions_submit_presets(ui, state, options)` requires the same
preflight-valid options and the caller's one open popup scope, after its Base
reduction and Calendar submission. It submits semantics only, never replays
input, predicates or callbacks. The mutable state receives bounded copied
previous-submission identity only after all submissions succeed; failure clears
its publication stamp. Duplicate helper calls are invalid Core submission.
Base itself submits trigger/clear semantics, not popup semantics.

The state stores the prior root/grid/month-count/week-start and at most 8 preset IDs,
plus at most 144 actual enabled semantic target IDs,
plus context-local prepared/submitted frame stamps. Omission, failed/missing
helper, reinitialization and close/reopen invalidate eligibility. Trigger toggles
and Escape invalidate the old popup action targets in the current action pass.
A genuine previous Core event target/activation is required alongside those
records and current policy/list membership. Scope Delete covers actual Calendar
navigation, month/year headers and selector/day grids, not only preset buttons.
No Core scope introspection or Surface dispatch changes are introduced. Caller
must reinitialize before moving state to another context or manually reopening
its overlay; a context pointer is not retained.


Final identity review: reserved year/month conflicts use decoded part intervals
and authoritative ID rechecks, not a 9999-year scan. Successful popup submission
copies at most 144 actual enabled semantic targets;
Scope Delete requires both that previous set and current view/grid/list eligibility.
An absent/off-view generated ID alone is insufficient (Core focus fallback is
described below). The
private overlay reducer reports open state after its trigger/dismiss pass, before
Calendar reduction, so outside-dismiss expiration is distinguished from the
explicitly retained Calendar auto-close/action phase behavior. There is no shared
Core routing change.

Scope Delete excludes the nonfocusable popup root and requires an actually submitted
enabled Calendar/preset semantic target plus current view/list eligibility. Tests
cover previously submitted year/day targets made off-view before reduction and
absence of unsubmitted IDs from the bounded record. Core redirects unknown
programmatic focus to an eligible modal target before key dispatch; that resolved
target, rather than the invalid requested ID, is authoritative.
