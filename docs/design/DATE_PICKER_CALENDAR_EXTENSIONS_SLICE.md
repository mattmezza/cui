# DatePicker Calendar extensions: audit and proposed contract

Design checkpoint only. No declarations, implementation, coverage promotion or new observer are supplied by this document. Baseline: CUI `fca994fd`. Proposed public names below require review before production.

## Frozen evidence and actual coverage

Reference checkout `/tmp/cui-gpui-reference`, exact commit `87d10ae5e1299d1be18670a37570c5402f952e34`:

| Source | SHA256 | Relevant contract |
| --- | --- | --- |
| `crates/base/src/date_picker.rs` | `995c7e32ad660e7f1e96d1c429e90374b6479bc9cd8364568683c8305bc8715b` | Controlled composition root; application supplies trigger/calendar/positioning. `on_open_change`, Confirm toggle and Cancel dismissal; ComboBox semantics. |
| `crates/component/src/time/date_picker.rs` | `e7c8c87c76b253703a955c578f28278212d54545d9c9a62a76892a3140095248` | Calendar subscription around lines 210–219, date selection/update around 403–431, child policy synchronization around 433–442, close/focus around 444–480, Calendar composition around 827–832. |

The frozen Styled picker composes the real Calendar, forwards its month count, first weekday and sizing, and shares the disabled matcher. Its date-only selection path updates the date, closes and emits its own Change; time editing can deliberately retain the popup. Base has a separate controlled open-change callback. This proposal covers access to CUI Calendar capabilities, not frozen date-time/preset/cleanable/appearance or arbitrary styling parity, nor an equivalent source event lifecycle. Calendar source evidence and its explicit policy differences are recorded in `CALENDAR_SELECTION_POLICIES_SLICE.md` and `CALENDAR_KEYBOARD_LOCALIZATION_AUDIT.md`.

Current CUI evidence:

- `include/cui/cui_picker.h:86–104`: caller-owned overlay plus Calendar state; DatePicker options already contain the complete legacy `cui_calendar_options`. Locale/custom labels, first weekday, 1–3 months, range, cell size, min/max and live disabled policy are already reachable. Year navigation is already present; a new API must not advertise these as newly implemented.
- `src/base/cui_picker.c:926–969`: DatePicker reduces Calendar through the zero-extension entry point, closes on accepted single selection or completed range, and submits the trigger's expanded/controls semantics. It does not submit popup Calendar semantics or own popup layout.
- `src/components/cui_picker.c:311–394`: Styled reduces before popup creation; formatting is locale-aware; popup submits and paints Calendar with zero presentation. Consequently month toggle, today marker, neighboring occurrences and behavior policies are currently inaccessible through this operation.
- `tests/unit/test_picker.c`, `date_picker_presentation`, `picker_year_escape`, `picker_events`: actual trigger/calendar glyphs and locale text, popup open/close, year Escape hierarchy, focus restoration, outside dismissal and disabled handling.
- `tests/unit/test_picker_events.c`, `dates`: legacy-vs-facade equality, Italian accessible date labels, same-date close without CHANGE, incomplete/completed ranges, keyboard queues, late viewport failure after mutation, callback close/focus timing, null/invalid IDs and omission. These are existing evidence, not proof of the proposed extension.
- `tests/unit/test_calendar_presentation.c` and `test_calendar_behavior.c`: extension reducer/semantics/paint and original-grid occurrence routing are tested as Calendar. Those results do not establish DatePicker popup integration.

## Recommended additive API

New authoritative header `include/cui/cui_date_picker_calendar.h`, including `cui_calendar_behavior.h` (which includes presentation and legacy picker declarations). Do not resize any legacy public record or modify existing function signatures.

```c
/* Proposed only. Value record; zero is the exact legacy DatePicker path. */
typedef struct {
    cui_calendar_presentation_options presentation;
    cui_calendar_behavior_options behavior;
} cui_date_picker_calendar_options;

cui_calendar_result cui_base_date_picker_ex(
    cui_context *, cui_id trigger, cui_id calendar, cui_str name,
    cui_date_picker_state *, cui_date_selection *, cui_date_picker_options,
    cui_date_picker_calendar_options);
bool cui_date_picker_ex(
    cui_context *, cui_id trigger, const cui_theme *, cui_str name,
    cui_date_picker_state *, cui_date_selection *, cui_date_picker_options,
    cui_date_picker_calendar_options);
```

The existing options remain authoritative for locale/month count/year navigation/disabled policy/viewport/width/placeholder. The new record composes existing presentation and behavior types without copying their fields into a third competing schema. Existing invalid-enum fallback, invalid/omitted host-today handling and single-month-only month toggle apply unchanged. No ambient clock, locale lookup, ownership transfer or allocation is added. Keep existing required-pointer and ID behavior; strengthening legacy ID validation is outside this slice.

Legacy functions delegate shared private bodies with a zero extension. Base calls `cui_calendar_reduce_with_behavior` once; Styled uses that Base body, `cui_calendar_submit_ex` and the existing paint helper with the same presentation record. Do not call a composed Calendar entry point after reducing: that would consume input twice and change close timing. The Base caller continues to position/open its overlay and submit Calendar with `cui_calendar_submit_ex` and the same presentation; no second reduction.

Styled keeps its legacy bool, including false on late popup/viewport failure after an applied edit. Base retains the full aggregate Calendar result. A richer Styled lifecycle result is a separately reviewed addition; do not silently reinterpret the existing bool as an event count or success code.

Optional helper to review with this API: `cui_id cui_date_picker_calendar_id(cui_id trigger)`, exposing Styled's existing private tag-900 Calendar ID for checked occurrence resolution and host integration. Return zero for trigger zero or a zero derived ID. No claim of collision impossibility; caller IDs retain the existing collision discipline. This is useful but not required for display: tests may inspect actual semantic IDs. A new helper must not reassign any legacy ID.

Alternative: take presentation and behavior as two trailing arguments, matching Calendar. This avoids a wrapper type but makes long DatePicker calls harder to scan. Enlarging `cui_date_picker_options` is rejected because it changes an existing by-value ABI; introducing a new Calendar reducer or copying its state machine is rejected because it creates divergent routing and phase rules.

## Ordering, close and focus contract

Preserve these stages exactly, including their existing limits:

1. If the popup began open in month/year view, the existing Escape prepass returns to days and focuses the year header before overlay dismissal. It scans the original event list; do not claim strict cross-stage chronology or that two queued Escapes equal two separate frames.
2. Update previous popup bounds, apply overlay dismissal, then process trigger activations in list order. Trigger toggles are a separate pass before Calendar reduction; arbitrary interleaved trigger/day events are not a new globally ordered dispatcher.
3. If still open, merge outer disabled into Calendar disabled and reduce Calendar once with the extension. Calendar preserves the original submitted grid/month plus date-and-panel target snapshot for its whole queue, while applying live pure policy checks per activation. Recenter must never reinterpret a queued cell as a different civil date.
4. After the full Calendar queue, close when `activated && (!range || !selecting_end)`. Do not close midway through the queue or truncate later Calendar activations. This final-phase aggregate rule differs from per-event close notification and remains unchanged.
5. Submit trigger semantics; Styled formats and paints its trigger, then either returns, closes on invalid viewport, or places/begins/submits/paints the popup. On newly opened popup it focuses the existing canonical cursor/year/month destination; close restores trigger focus through overlay logic. Preserve modal scope removal before further submission.

Explicit policy consequences: earlier pending endpoint restart keeps the popup open; accepted completion closes; a rejected completed candidate with a newly disabled old start leaves both selection/phase and popup unchanged. Same-date single activation closes even without a value change. Year/month navigation and today decoration do not close or emit CHANGE. Opening must not reinitialize caller Calendar state, force days, recenter selection or resynchronize `selecting_end` merely because the new endpoint-shape policy was supplied. Changing policies while closed has no activation effect.

Outer disabled closes the popup through existing overlay policy; Calendar-only disabled leaves the trigger operable and suppresses Calendar actions. Read policy resources live under the existing pure-predicate contract. If omitted, this component does not process input or synthesize dismissal. State/options/locale strings/predicate resources remain borrowed through the existing frame/render-consumption boundary. Extension values are copied and not retained. Base layout and Styled viewport clipping remain separate contracts, including bounded month geometry near years 1 and 9999.

## Observer and lifecycle gap: separate work

Existing `cui_*date_picker_events` calls the legacy outer operation once, then compares final endpoints and optionally invokes one borrowed CHANGE callback. Styled notification occurs after painting, close/focus restoration and any late failure. Metadata target is the trigger, source UNKNOWN, with proven active-frame metadata only. It may notify an actual applied value change even when the legacy Styled bool is false after an invalid viewport. An incomplete range can notify; a completed same value, phase-only change, opening or dismissal cannot. Callbacks must defer teardown/UI mutation; unrelated application flags may change.

Those wrappers must remain on the zero-extension path in this first slice. Do not wrap a Calendar observer inside DatePicker or claim that a returned aggregate result observes lifecycle. A future `*_ex_events` could reuse the same final-value CHANGE payload/timing around the extended outer operation, but requires explicit tests and separate approval. Frozen Base open-change and Styled date-only/time-completion notifications are not represented by this payload. Open/close reasons, per-activation delivery, exact pointer/key/AT causes, cancellation and controlled/rejectable requests require an independently designed lifecycle facade; they are not covered by this proposal.

## Implementation ownership and acceptance evidence

Production would need the new header, Calendar/DatePicker-only private seams in `src/base/cui_picker.c` and `src/components/cui_picker.c`, a new `tests/unit/test_date_picker_calendar.c`, worker fixture and compiling guide. Do not touch ColorPicker/time fields. Canonical CMake, distribution implementation entry mapping, SDK source copies, metadata/usage mirror/skill and WASI registration follow integration ownership. Builders/FFI have no new record marshalling in this slice; a packaged C header is not builder support.

Required headless tests before claiming coverage:

- Zero extension vs legacy: Base result/state/selection and Styled bool, semantic IDs/names/expanded/controls, focus, actual draw commands and software pixels; both themes, closed/open, 1–3 months, each view. Unknown behavior enum fields independently fall back while valid sibling fields work.
- Actual pointer, Enter, Space release and AT trigger opening, header/month/year navigation, neighboring date activation, canonical roving focus and selection close/focus restoration. Original-grid queued duplicate/neighbor targets remain stable across recentering. Compare final-phase queue behavior with legacy, including complete-then-start and net-zero endpoint queues.
- Restart/swap and completed-endpoint validation independently and in combinations; live newly disabled anchor, allowed disabled interior, phase-shape disagreement, same-date close, incomplete remains open and rejected completion remains open. Recenter and year-to-days preserve selection and are silent for existing CHANGE regressions.
- Locale/custom-label trigger and popup semantics/paint; host today disabled/selected precedence; month toggle only one month; year extremes 1/9999, leap cursor clamp and disabled-month/year focus fallback; popup width/clipping using actual viewport bounds.
- Escape hierarchy across frames and documented same-frame prepass; outside dismissal, repeated trigger toggles, disabled-vs-Calendar-disabled, layout/inert/omitted component, null inputs, invalid viewport after mutation and no modal scope left behind. Regression observer callbacks retain exact legacy final-state/focus timing and metadata.
- Strict C11 Release via private Xvfb wrapper; compiled C usage, C++11 declarations, standalone generated-header entry and pinned WASI suite. Browser extension coverage remains unclaimed until an actual headless WASM fixture passes.

This is a cohesive forwarding/composition slice, not a DatePicker parity completion. No production changes are authorized by this document itself.
