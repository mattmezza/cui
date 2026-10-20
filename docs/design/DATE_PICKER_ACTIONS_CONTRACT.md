# DatePicker date-only actions: proposed contract

Review checkpoint only, based on `b0405a3c`. No declarations below are installed,
no production implementation is included, and no tests are claimed executed.
Scope: presets and clear actions, legacy-preserving Base/Styled composition;
no date-time, new visual variants, lifecycle observers or Core dispatch changes.

## Proposed exact declarations

New `include/cui/cui_date_picker_actions.h` includes
`cui_date_picker_calendar.h`. All declarations use portable C11 existing types.

```c
#define CUI_DATE_PICKER_PRESET_LIMIT 8u
#define CUI_DATE_PICKER_LABEL_LIMIT 96u

typedef enum {
    CUI_DATE_PICKER_SINGLE = 0,
    CUI_DATE_PICKER_RANGE
} cui_date_picker_shape;

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

typedef struct {
    cui_calendar_result calendar;
    bool valid;
    bool changed;
    bool action_activated;
} cui_date_picker_actions_result;

cui_date_picker_actions_result cui_base_date_picker_actions(
    cui_context *, cui_id trigger, cui_id calendar, cui_str name,
    cui_date_picker_state *, cui_date_picker_value *,
    cui_date_picker_actions_options);

cui_date_picker_actions_result cui_date_picker_actions(
    cui_context *, cui_id trigger, const cui_theme *, cui_str name,
    cui_date_picker_state *, cui_date_picker_value *,
    cui_date_picker_actions_options);
```

No new init is necessary: initialize embedded picker state with existing
`cui_date_picker_init` and initialize the new caller-owned value explicitly.
The existing `cui_date_selection` cannot store source-preset shape changes;
`value.shape` is therefore authoritative. The facade substitutes it for
`options.picker.calendar.range`; that legacy field is deliberately ignored here,
and all other picker/Calendar options are forwarded. No hidden mode in global or
retained state is permitted. Legacy callers and public record layouts are unchanged.

## Defaults and eligibility

Zero options have no presets, clear affordance or Delete action. The old functions
are unchanged; new zero-action calls produce the same selection/state/rendering
as the corresponding `_ex` call after substituting the value's explicit shape.
Zero preset policy matches frozen source's shape-changing behavior: a valid
single preset can replace a range and vice versa. Source policy bypasses
Calendar date eligibility, as the upstream handler does; it does not bypass basic
date validity or bounded preflight. `MATCH_SHAPE` rejects a mismatched preset
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

An enabled clear removes both dates, preserves shape and displayed Calendar
month/view/cursor, resets `selecting_end`, closes the overlay, and restores trigger
focus only if it was on the trigger or within the popup. A valid preset replaces
shape/dates, resets `selecting_end`, closes, and uses the same focus rule. It does
not recenter the Calendar: the frozen preset handler does not establish a
recenter requirement. An empty clear still counts as accepted action and may
close, but `changed` is false. A preset equal to current shape/dates similarly
counts as accepted action without fabricated value change. No event emission
semantics beyond the return record are introduced.

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
no state/value/focus changes, consume no events, submit no control, and call no
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
predicate. Only actions targeted to the prior submitted clear/trigger/open popup
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
ordering, first accepted action wins, stale closed-popup IDs, outside-focus
preservation and net-zero edits. Include software pixels and semantic names/
expanded/controls for the visible clear/preset controls; no live AT claim.
Compile C11/C++ header probes and strict warnings. Run fixture via private Xvfb
wrapper with meaningful sanitizers. Root owns canonical registration, bindings,
metadata, skill and generated distribution updates after reviewing this contract.
