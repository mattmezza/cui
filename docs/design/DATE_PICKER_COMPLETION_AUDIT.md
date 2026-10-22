# DatePicker completion audit

Design checkpoint only, 2026-10-20. CUI baseline is
`3ddd91bad33a16c3732f0fd05482f2cbf7126783`; frozen GPUI Kit 0.7.1 is
`87d10ae5e1299d1be18670a37570c5402f952e34`, verified in the local reference
checkout. No runtime, public header, canonical registration or parity status changes
are authorized by this document. Preserve all 78 catalog entries.

## Source authority

Frozen source links below point to the exact revision, not moving documentation:

- [Styled DatePicker](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/time/date_picker.rs): `DateTime` (52), `DateRangePresetValue` (113), state/builders (153–395), date selection/clear/preset transitions (403–538), Styled builders/render (542–866).
- [Base DatePicker](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/date_picker.rs): controlled root (17–55), child/style composition (58–76), ComboBox semantics, Confirm toggle and Cancel dismissal (80 onward).
- [Upstream tests](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/kit/tests/date_picker.rs) are secondary evidence; implementation controls the distinctions below. This audit does not claim that upstream Rust tests were executed.

CUI authority is [picker declarations](../../include/cui/cui_picker.h),
[Calendar forwarding contract](../../include/cui/cui_date_picker_calendar.h),
[Base reducer](../../src/base/cui_picker.c) and
[Styled composition](../../src/components/cui_picker.c), all at the stated baseline.

## Feature matrix

“Implemented” means the stated CUI capability exists, not that all upstream
behavior, bindings or live assistive-technology behavior is equivalent.

| Frozen requirement | Current evidence | Status / remaining boundary |
| --- | --- | --- |
| Single/range values, popup trigger, outside dismissal, disabled control | `cui_date_selection`, `cui_date_picker_state/options`; Base lines891–989; Styled321–404 | Implemented date-only foundation. Single acceptance/completed range closes; partial range remains open. Does not include time-mode close behavior. |
| Locale, first weekday, multiple months, min/max and disabled-date matcher | `cui_calendar_options`, `cui_date_locale`, locale formatting; forwarded by both DatePicker paths | Implemented, with CUI's documented numeric locale formatting and bounded displayed-month policy. Not an OS locale/time-zone provider. Do not relabel these missing. |
| Year/month navigation, year bounds, today/neighbor cells and range policies | `cui_date_picker_calendar_options` carries `cui_calendar_presentation_options` and `cui_calendar_behavior_options`; Base978, Styled395 | Implemented opt-ins. `_ex` forwards once and preserves legacy defaults. Today is supplied by the host, not read from a clock. Year destinations/range restart/endpoint validation/recenter policies are explicit. |
| Preset labels and single/range/date-time choices | Upstream113–150,508–526,638–641,806–822; CUI picker records have no preset list/action | Missing reusable DatePicker feature. An app can compose buttons and assign values, but that supplies neither standardized validation nor close/focus/event semantics. |
| Single-date time precision, hour cycle and default time | Upstream261–291,359–393,403–416,667–697,848–854; CUI `cui_time`, `cui_time_state/options`, `cui_time_field` exist separately | Partial building blocks; integrated date-time value, popup time field, precision truncation and time-aware Change are missing. A separate TimeField is not DatePicker completion. |
| Clear affordance and Delete action | Upstream cleanable defaults false (604), builder630, clean484–499, on_delete459; CUI options only calendar/viewport/placeholder/disabled/width | Missing integrated clear button/key behavior. Empty date values are representable and host-clearable. No current clear action resets range phase/time or emits a DatePicker change on its own. |
| Custom trigger | Upstream Base owns arbitrary children/style and controlled open; Styled DatePicker exposes no custom-trigger builder | Partial/composable: CUI `cui_base_date_picker_trigger` submits semantics and toggles caller state; caller owns layout/paint. Full Base reducer and split Calendar submission permit custom composition. Styled trigger is fixed. Do not invent an upstream Styled custom-trigger API requirement. |
| Appearance/size/style/focus-ring controls | Upstream `Styled`, `Sizable`, `FocusableExt`, `appearance` (569–589,650–654); CUI Styled335–351 fixed outline button/calendar icon | Partial: theme, width, calendar cell size and Base custom painting exist. No equivalent DatePicker appearance flag, per-control size/focus-ring override or arbitrary styled refinement. |
| Placeholder and displayed format | Upstream `placeholder`, `date_format` and `display_format`; CUI placeholder + locale date order/separator | Placeholder implemented. Arbitrary date/time patterns and formatted date-time/range title missing. Locale formatting is explicitly not an arbitrary pattern parser. |
| Change and open lifecycle | Upstream Change carries `DateTime`; Base `on_open_change`; CUI `cui_picker_events.h`, Base/Styled picker event implementations | Date-only aggregate CHANGE exists. `_ex` Calendar forwarding has no corresponding extended typed wrapper; open/close notifications and date-time payloads missing. No common observer or dispatch redesign is implied. |
| Accessible trigger/popover relationship | Upstream Base ComboBox, expanded/value; CUI Base921–925,966–970 uses Button + expanded/controls, localized label | Partial semantic correspondence, not absence of semantics. No current claim of matching ComboBox/value model or live AT announcements. Adapter/DOM/live AT evidence must be evaluated separately. |

## Frozen behavioral constraints that affect design

The source only edits times when `time_precision` is configured **and the current
value is single**, via `edited_time_precision`. A range can store start/end times
through `set_date_time` or a date-time preset, but the popup does not edit both
range endpoint times. Two single pickers are the upstream recommended composition.
No timezone-aware or daylight-saving conversion requirement follows from its
`NaiveDateTime` model. CUI should retain calendar dates and wall-clock times, with
host-owned timezone conversion, rather than inventing a UTC/OS clock dependency.

In single time mode a changed day emits immediately and leaves the popup open;
activating the same selected day confirms/closes without another value edit.
Time-field changes emit through `emit_change`, which only emits a complete value.
Date-only `update_date` closes and emits. Clear restores default times, empties
both endpoints according to current shape, and closes through `update_date`.
Delete is wired on the root independently of whether the cleanable button is
visible: `cleanable` controls the visible affordance, not that action registration.
A future CUI clear-key policy must explicitly choose/document whether to reproduce
this distinction, including empty/no-op Change behavior and disabled gating.

Preset buttons are rendered with `tab_stop(false)` upstream. Their transitive
state updates matter: `select_preset` calls `update_date` (or `set_date_time`,
which calls it), which first changes the picker's independent date and then calls
child Calendar `set_date`. In frozen `crates/base/src/calendar.rs:217–235`,
`apply_date` rejects a matched candidate without changing the child date or view;
otherwise it changes the child date and recenters current month/year to a present
start. It does not reset Day/Month/Year view, year page or other cursor state.
`Matcher::is_match` (118–124) checks a present single or both complete range
endpoints, not the interior. Thus a disabled preset can change the outer picker
value/title and emit Change while retaining the previous child selection and
month/year. This is independent retained child ownership, not atomic rejection
of the picker action. `select_preset` then unconditionally focuses the trigger.

Clear also calls `update_date`, after restoring default times. Empty single/range
values do not match `Matcher::is_match`; the child date therefore empties as well.
No start exists to recenter, so current month/year and Calendar view remain.
`clean` does not explicitly focus the trigger, unlike `select_preset`; it calls
neither `set_open` nor conditional `focus_back_if_need`. Its actual event focus
can depend on clear-button/root handling and must not be described as an explicit
focus restoration guarantee. A future CUI facade sharing one caller selection
cannot reproduce rejected-child divergence without separate retained child value
state. Alternatively it must document a deliberate atomic-eligibility policy
rather than call it source-equivalent. Relative presets receive resolved dates
from the host; no clock access is required.


## Recommended additive slices

1. **Date-only actions and composition first.** Introduce a separate extension
   header/entry points and value-only options, leaving `cui_date_picker_options`,
   state layout, old calls and zero-extension behavior unchanged. Define bounded,
   caller-owned preset records with explicit single/range shape, labels and stable
   action IDs; clear is its own target, not trigger activation. Specify capacity,
   duplicate IDs, invalid UTF-8/invalid dates, disabled policy, mismatch between
   preset and picker shape, close/focus behavior and whole-queue ordering before
   implementation. Borrow arrays/labels only through submission/render consumption;
   never retain handlers or invoke UI from predicates. A small bounded preset list
   and clear affordance are preferable to an unbounded list with implicit scrolling.
   Reuse the existing Calendar extension in the new facade rather than losing its
   policies. Ship an actual compiling Base custom-trigger example with the same
   reducer, overlay and focus contract; no arbitrary C callback renderer is needed.
2. **Styled presentation and typed wrapper.** Add explicit minimal/outline and
   supported size/focus presentation options without promising Rust-style arbitrary
   refinements. Use existing theme/Button primitives, preserve clear hit targets,
   keyboard focus and trigger controls/expanded linkage. Add an extended aggregate
   date CHANGE wrapper with exactly the old post-reduction notification restrictions.
   Decide open/close event scope separately; comparing caller-owned `overlay.open`
   is already possible but is not a lifecycle subscription API.
3. **Single date-time composition.** New explicit value/state/options records can
   combine `cui_date` and `cui_time`; do not append time fields to legacy date
   selection/event layouts. Reuse TimeField and define minute/second precision,
   default-time truncation, 12/24-hour presentation, external updates, draft
   preservation, repeat-day confirmation, clear and preset time reset/preservation.
   Keep range time editing out of this first slice. Define a dedicated date-time
   event payload and bounded formatter contract; a supplied formatted title or
   narrowly specified formatter avoids introducing a general chrono-pattern parser.
   Add range date-time storage only with a separately reviewed model contract.

No new Core state machine, platform types, networking, allocation service or new
dependency is required by the first slice. Public types remain portable C11. Base
must continue to reduce once; Styled must paint/submit that result, not reduce a
second time. Existing `_ex` behavior permits a late invalid viewport/overlay
failure after a selection edit; new wrappers must not mistake the legacy boolean
for a transaction rollback. Updating shared headers requires matching distribution
sources/bindings and integrator-owned metadata/skill regeneration in production.

## Meaningful acceptance evidence

Existing source tests inspected: `tests/unit/test_picker.c` covers locale models,
year grids, picker Escape, custom locale presentation, picker presentation and
trigger interactions; `test_picker_events.c` covers aggregate notification behavior;
`test_date_picker_calendar.c` covers range policies, queue validation, localized
headers, neighbors, paint/boundaries, zero/invalid options, lifecycle/errors and
combinations. `tests/test_calendar_distribution.py` checks additive declaration/
implementation distribution. These support the existing Calendar capability claims,
not presets/time/clear claims. Existing web locale/year and Calendar presentation
fixtures do not establish missing DatePicker actions or live AT behavior.

For slice1 require headless Base/Styled parity tests for pointer, keyboard and
semantic activation; clear on empty/single/partial/completed range; no trigger
retoggle after clear; mixed clear/preset/day queues; disabled/no-op/rejected cases;
focus restoration versus outside focus; stale IDs; borrowed-label consumption;
viewport failure after edit; and old/zero-extension state+semantic+software-pixel
comparisons. Check preset endpoint policies explicitly. Slice2 adds actual
headless browser hit-target/focus/popup checks and event ordering; do not substitute
fake DOM snapshots for the actual WASM path. Slice3 adds actual TimeField draft,
precision rollover/truncation, date preservation, same-day confirm and complete
payload tests, including externally changed values while a draft is active.

This checkpoint is a source audit, not a new execution claim: no full build,
native window, browser, upstream test suite or AT session was run. Future native
tests must use `scripts/test_local.py` private Xvfb; browsers remain headless.
