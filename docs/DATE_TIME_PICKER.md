# Single-date time picker

This opt-in portable C11 API composes one civil date and time. It implements the
Base/value/synchronization and Styled slices of `design/DATE_PICKER_DATETIME.md`.
No timezone/range editor, arbitrary date parser/formatter, typed event
wrapper or binding builder is included. Existing
DatePicker/actions and TimeField calls keep their behavior.

Include `cui/cui_date_time_picker.h`; link `src/base/cui_date_time_picker.c` along
with the normal picker/actions/Calendar sources. Canonical builds register the Base and Styled sources, standalone optional
`CUI_DATE_TIME_PICKER_IMPLEMENTATION` and `CUI_IMPLEMENTATION_ALL` distribution
paths. Native source SDKs carry the API; language builders do not expose it.

## Ownership and frame order

Retain `cui_date_time_picker_state` and `cui_date_time_value` for one UI context,
on its owner thread. Initialize with a stable popup ID and initial valid month.
NULL init is a no-op; all-zero/uninitialized state cannot reduce or assign.
`cui_date_time_value.date` is a valid Gregorian date or all-zero absence; partial
invalid dates reject preflight. `time` must always be valid 00–23:00–59:00–59,
even with no date. It exposes retained time that upstream's empty DateTime hides;
check date presence before using it as a committed date-time.

Call `cui_base_date_time_picker` once per frame, passing stable distinct trigger,
Calendar and TimeField IDs plus state/value/options. It reduces once, submits
trigger/optional clear semantics, and returns result flags. Caller owns all paint
and the popup. Do not call public `cui_time_field` or `cui_base_time_field` again:
those functions both reduce and submit.

For an open popup, the exact order is:

1. Set bounds and begin the existing `state.actions.picker.overlay` once; begin
   the caller's portal/layout inside that scope.
2. `cui_calendar_submit_ex` using `state.actions.picker.calendar` and retained
   `state.actions.calendar_value.dates`, with the same Calendar options/extensions.
   Combine outer disabled into Calendar disabled. This submits without reducing.
3. `cui_date_time_picker_submit_time(ui,time_id,&state,&value,options)`.
4. `cui_date_time_picker_submit_presets(ui,&state,options)` even if preset_count=0.
5. Paint/layout these submitted IDs; end the portal and overlay scope once.

Both helpers are semantics-only and require this frame's successful Base call,
the same valid options/value and the caller's real open popup scope. Time submission
must precede preset submission: the latter publishes bounded actual Calendar,
preset and TimeField target identities for the next frame. Time segments require
both successful helper stamps from the immediately preceding frame; omission,
failed/duplicate helper, omitted frame or close/reopen expires admission. Helpers
never replay input/state edits/callbacks. Core capacity errors after submissions
are not rolled back; failed helper stamps publish no partial identity set.

`cui_date_time_picker_time_id(trigger)` supplies a stable optional derived time
root; Base also accepts a caller-supplied time ID. Segment IDs are the existing
`cui_time_segment_id(time_id,0..3)`. All four are reserved during preflight,
including currently hidden second/AM-PM segments. Caller global ID discipline is
still required. Existing modal outside-clear restrictions apply: dismiss the
popup before using the separate clear button outside it.

## Values, synchronization and drafts

MINUTE truncates seconds to zero; SECOND retains them. Twelve-hour appearance
never changes civil storage. No timezone, UTC offset, DST, leap-second or implicit
clock service exists. First valid reduction uses supplied time for a present date,
and configured default time for an absent date. Options are validated atomically
before predicates, normalization, input or state changes.

Repeated unchanged controlled values preserve draft digit count/segment, Calendar
view and rejected-child divergence. A changed external value is setter intent:
reset draft, truncate supplied time (empty assignment uses default), sync child
if eligible, close directly and emit no source Change. Between frames,
`cui_date_time_picker_assign(&state,&value,next)` queues identical setter intent
as well: it validates next atomically and sets a pending flag, consumed only by a
valid Base call. Invalid options leave pending assignment intact. It does not
require a UI context or touch focus/semantics between frames.

Default-time changes reset current time; precision changes truncate it; hour-cycle
changes reset draft while preserving civil time. Changes apply once before input;
external setter intent wins over simultaneous default changes. Ordinary close,
reopen and idle frames do not push a time value into a pending draft. Disabled
popup omission retains draft but rejects user input; Calendar-only disabled gates
day edits and does not disable the time field, presets or clear.

## Source behavior and results

Frozen authority is `87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/component/src/time/date_picker.rs:215–227,261–291,327–447,484–538`.

- Different accepted day adopts child into outer date, preserves time and stays
  open. Same accepted day closes and confirms without a source Change. Comparison
  uses outer date: an unchanged retained child can still replace a divergent outer.
- Actual TimeField change stays open, preserving date. A present date reports
  source Change; an absent date only changes retained time storage.
- Date-only preset preserves current time. Timed preset replaces/truncates it and
  resets draft. Both close, emit source Change even if identical and explicitly
  focus trigger. SOURCE may accept outer date despite child matcher/bounds
  rejection; MATCH_SHAPE_ENABLED pre-rejects ineligible assignments. Shape matching
  alone is vacuous for this single-only facade. Changing matcher or reopening
  does not silently resynchronize rejected child; an actual later day edit does.
- Clear/Delete restores default time, clears outer/child date and draft, preserves
  Calendar view/month, closes without an explicit focus request, and reports source
  Change even if already empty. `cleanable` controls visibility independently of
  Delete OFF/TRIGGER/SCOPE. Outer disabled gates every user action.

`valid` is preflight success, not rollback of later Core/layout failures.
`value_changed` compares final date AND time against entry storage, including
normalization/external-empty/default changes. `source_change` coalesces accepted
source Change causes; it is not a callback stream or Change count. Repeated preset
or clear can report source_change=true with value_changed=false. A time edit then
preset can restore the entry value with source_change=true. An empty-date time
edit has value_changed=true but source_change=false. `time_edited` means the
TimeField actually changed storage; `action_activated` means accepted clear/preset;
`confirmed` means same-day confirmation. `calendar` is the one reduction result.

Phase order: configuration/external sync; Escape/overlay/trigger pass; Calendar;
TimeField if still open and admitted; first eligible action in queued order.
There is no global chronological interleaving of Calendar/time/action events.
Escape/trigger transitions invalidate old popup action/time admission. Scope
Delete accepts only actual previous submitted enabled targets still eligible in
the current view/list/precision. Popup root or generated namespace IDs alone do
not qualify. Core may redirect invalid programmatic focus before key dispatch;
the resolved real target is authoritative.

## Validation and lifetimes

Preflight bounds: at most 8 presets, 96-byte nonempty scalar UTF-8 action labels;
empty time_label falls back to Time. Reject malformed active time, date, precision,
policy, dimensions/viewport, min/max, range=true, duplicate/reserved IDs, invalid
active UTF-8 or NULL preset storage before mutation/predicates/input. Ignored
has_time=false preset time is not inspected. Bounds/predicate checks on assignment
are inherited CUI extensions; predicates are pure, owner-thread only.

Arrays/options are borrowed for calls. Strings, locale tables and predicate/user
resources remain valid through frame/render consumption. State stores copied
values/IDs only and allocates nothing. Reinitialize before moving to a different
context; do not edit bookkeeping, manually force reopen or reuse state roots in
the same frame. Legacy public record layouts stay unchanged; new record users
must rebuild with matching headers.

Private fixture: `cmake -S tests/fixtures/date_time_picker -B BUILD -G Ninja`,
then build `test_date_time_picker test_date_time_picker_styled test_picker
test_date_picker_actions date_time_picker_header date_time_picker_guide date_time_picker_guide_cpp`.
Run through `python scripts/test_local.py --build-dir BUILD -- -V -R
'^(date_time_picker|date_time_picker_styled|picker|date_picker_actions)$'`. Repeat in a separate
`-DCUI_SANITIZE=ON` build. `tests/fixtures/date_time_picker/test_wasi.py --build-dir
BUILD` uses the repository's existing pinned offline WASI driver. The Base/Styled checks alone add
no live-AT or full source-parity evidence. Final worker results will be
reported with the signed checkpoint. Configure a separate fixture with
`-DCUI_DATETIME_BUILD_STYLED=OFF` to build/run the Base test without the optional
Styled source.

Accepted Calendar selections are evaluated in queue order against the immediately
preceding outer date. A→B→A remains open with source_change true and
value_changed false. A→B→B confirms B; A→A→B confirms A and skips later
Calendar events. The private synchronous visitor uses constant stack storage;
close occurs once after Calendar reduction. Legacy Calendar entry points have
no visitor and preserve their existing queue behavior. This stop-first-confirmation
rule describes CUI routing, not GPUI scheduler equivalence.

TimeField source_change uses the existing reducer's net phase result: Up then
Down restoring the entry time does not report a time source change. There is no
ordered callback stream or per-intermediate-time-edit notification claim.


## Styled composition

`cui_date_time_picker` calls Base once and owns the single popup scope, Calendar,
paint-only TimeField, and preset/clear paint. Use this instead of manually calling
Base/helpers in that frame. Calendar root is `cui_date_picker_calendar_id(trigger)`;
time root is `cui_date_time_picker_time_id(trigger)`. No extra resources are retained.
The trigger displays a bounded locale date plus 12/24-hour minute/second time;
there is no chrono pattern API. This display is separate from its accessible
name supplied by the caller. `time_label` labels both the visible row and group.

Styled validates theme metrics and rejects a popup larger than the supplied
viewport before reducing input or mutating value/state. It does not silently
clip away time/actions or add scrolling. Supply enough room, fewer months/presets,
or smaller Calendar cells. DatePicker has no `design_size` field: actual supported
metrics are Calendar cell_size (20–128, default32), 1–3 months, and finite theme
font/padding/gap/radius; XS–9XL enums from other components are not accepted here.
A bounded formatter can truncate very long localized date labels; custom source
format patterns remain unsupported. Native/browser text shaping follows the
existing host; software geometry tests omit glyph rasterization without a font.

The following complete C11/C++11-compatible example is compiled by the standalone
fixture. The application calls init once, retains both objects, and calls frame
inside its own begun CUI frame. It links the Base source plus optional
`src/components/cui_date_time_picker.c` and ordinary components/Calendar sources.

<!-- datetime-example-begin -->
```c
#include "cui/cui_date_time_picker.h"
#include <string.h>

void example_datetime_init(cui_date_time_picker_state *state,
                           cui_date_time_value *value) {
    cui_date initial;
    initial.year = 2024; initial.month = 2; initial.day = 15;
    memset(value, 0, sizeof *value);
    value->date = initial;
    value->time.hour = 9; value->time.minute = 30;
    cui_date_time_picker_init(state, 702, initial);
}

cui_date_time_picker_result example_datetime_frame(cui_context *ui,
    const cui_theme *theme, cui_date_time_picker_state *state,
    cui_date_time_value *value, cui_rect viewport) {
    cui_date_time_picker_options options;
    cui_date_time_preset preset;
    memset(&options, 0, sizeof options);
    memset(&preset, 0, sizeof preset);
    preset.id = 704; preset.label = cui_string("Tomorrow");
    preset.date.year = 2024; preset.date.month = 2; preset.date.day = 16;
    options.picker.viewport = viewport;
    options.precision = CUI_DATE_TIME_SECOND;
    options.default_time.hour = 8;
    options.presets = &preset; options.preset_count = 1;
    options.cleanable = true;
    options.clear_id = 703; options.clear_label = cui_string("Clear");
    options.delete_policy = CUI_DATE_PICKER_DELETE_SCOPE;
    return cui_date_time_picker(ui, 701, theme, cui_string("Appointment"),
                                 state, value, options);
}
```
<!-- datetime-example-end -->

The local preset array is borrowed only during submission; its literal label
remains readable for rendering. Application-owned dynamic strings must follow
the frame lifetime rules above. Bindings remain separate; the gallery mode below uses the C API directly.

Styled slice validation: focused native and ASan/UBSan wrapper checks each pass
4/4 suites; pinned WASI passes Base579 and Styled695 assertions. Base-only
configuration passes its 579 assertions without the optional Styled source.
Software tests cover 1–3 months, minute/second and 12/24-hour segments, actual
popup containment, supported cell metrics/theme variants, eight presets, narrow
rejection, draft/Tab/preset/clear behavior, and legacy TimeField memberwise
semantics/nontext pixel equivalence. Existing date-only actions tests retain
legacy zero-option picker equivalence. Guide/header compile as C11/C++11.
These are headless software/semantic checks, not browser, live-AT or GPU proof.


Canonical integration additionally compiles and executes the exact guide example
in the native and WASI Styled suite (701 assertions including that example).
Standalone optional-module and CUI_ALL fixtures create a real popup, edit its
TimeField, and confirm the selected date. The packaged Python source tree omits
docs, configures and builds both native DateTime tests, and runs them through the
private-display wrapper. Consumer SDK/distribution headers are generated; language
builder/bridge omissions remain explicit above. This stays within the existing
DatePicker family; it does not add a 79th manifest entry or claim complete parity.


For standalone header implementation, emit the ordinary Core, Base, Theme and
Components implementation macros once, then `CUI_DATE_TIME_PICKER_IMPLEMENTATION`
and include `cui_date_time_picker.h`. The exact independent recipe is compiled
and executed in `tests/distribution/date_time_picker.c`; its CUI_ALL mode uses
`CUI_IMPLEMENTATION_ALL` instead. Declarations alone do not enable implementation.


## Browser gallery mode

The DatePicker story defaults to its existing date-only presentation. Enable
**Date + time** for the single civil date/time facade; only then do Include
seconds, 12-hour clock and optional clear/Delete controls appear. Configure those
controls while the popup is closed. Date-only actions are hidden during time mode
and restored when it is turned off. The current date transfers between modes;
entering time mode retains its last time, while clear restores 09:30 and precision
reduction permanently drops seconds. Time mode is single-date and discards a
range end; the gallery does not offer a date-time range control.

Launch day preserves time; Timed review replaces date/time (and truncates seconds
at minute precision). A changed day leaves the popup open for time editing;
selecting that same day confirms. Clear/Delete empties the date and resets time.
The visible date/time label follows the selected date locale and time appearance;
Time/segment names and preset labels are explicit English sample strings. The
copied complete C11 snippet reflects precision, period, clear/Delete and locale.
No timezone conversion or localization service is involved.

`tests/web/test_date_time_picker_gallery.py` uses actual headless Chromium mouse/
keyboard and CDP touch input to exercise numeric drafts across idle frames,
seconds/AM-PM, changed/same-day confirmation, timed/date-only presets, clear and
scope Delete from a time segment, disabled state, EN_US/FR and mode transfers.
It copies and strictly compiles the displayed snippets. Existing date-only
presentation/actions desktop/touch tests remain passing. This is sample/browser
host evidence, not Safari/mobile-OS keyboard/live-AT or full event-stream parity.
