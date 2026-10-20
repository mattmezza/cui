# Calendar neighboring-date occurrence design

Proposal only, independently reviewable after implementation commit `f7ce2aed`. No neighboring-date API or behavior is implemented by this checkpoint. Frozen authority remains GPUI `87d10ae5e1299d1be18670a37570c5402f952e34`; see `CALENDAR_KEYBOARD_LOCALIZATION_AUDIT.md` for exact source digests.

## Source-backed gap

Frozen `crates/base/src/calendar.rs:741–784` renders each date in each displayed month's week rows. Outside-month dates are muted, not automatically disabled. The predicate still controls activation. The occurrence ID includes date, panel offset and week index. Each day receives an ISO full-date accessible name. Component paint applies muted foreground before range/selected/today decoration (`crates/component/src/time/calendar.rs:174–214`). Source `apply_date` (222–234) recenters to the selection's start date, including after range changes. Its earlier second range endpoint restarts the range rather than swapping endpoints.

CUI already calculates all 42 cells, but semantics and Styled paint omit outside-month dates. `cui_calendar_day_id(root,date)` is intentionally date-based, so reusing it for every visible cell would create duplicate semantic/layout IDs across panels. The private target resolver currently scans neighboring dates too, but that does not provide a submitted, painted or accessible neighboring cell. The new month-toggle/today slice did not change this behavior.

## Recommended additive contract

Add a zero-default `neighboring_days` opt-in to the new presentation record, with an explicit rebuild note if its layout changes after publication. Keep legacy `cui_picker.h` and existing functions unchanged. If root has already frozen the new record ABI, use a separately versioned extension record instead; do not silently change installed ABI. The current one-to-three month bound and six-row Gregorian grid remain.

A context-free query helper should return the actual cell's date and ID, so hosts, tests and Base composition do not reproduce private hashing:

```c
/* Proposed only; names and signature require review. */
typedef struct {
    cui_date date;
    cui_id id;
    bool neighboring;
} cui_calendar_cell_info;
bool cui_calendar_cell_resolve(cui_id root, cui_date first_month,
    unsigned first_weekday, unsigned panel, unsigned cell,
    cui_calendar_cell_info *output);
```

Inputs require a nonzero root, valid first month, panel 0..2 and cell 0..41. Normalize the first month's day to 1 and first weekday modulo seven, matching existing grid behavior. Return false with zero output for invalid inputs, invalid boundary cells or unrepresentable panel months; NULL output also returns false. No allocation, context, locale or callback. A cell query describes geometry, not whether the caller enabled neighbors or submitted that panel. No platform types enter the API.

In-month cells keep **exactly** `cui_calendar_day_id(root,date)`. Neighboring cells use a dedicated private namespace encoding date plus panel, avoiding existing date/month/year/navigation namespaces. Encode within checked/bounded integer arithmetic; document that IDs are root-scoped and stable for the same date/panel, and that neighboring and canonical occurrences intentionally differ. Do not use only cell index: after navigation the same index must not activate a different civil date from an already queued event. Test namespace collisions exhaustively across all 126 cells in representative months and boundaries, and verify practical nonzero IDs under supported root constraints (the existing XOR-derived IDs can theoretically produce zero for particular roots; either follow the library's documented ID collision constraints honestly or introduce an explicit checked query failure, never claim mathematical global uniqueness).

Alternative: return occurrence IDs for every cell, including in-month dates. It is simpler internally but breaks the established day-ID contract and focus integrations, so reject it for this compatible slice. Rendering neighbors as passive labels avoids duplicate IDs but fails the frozen selectable-neighbor contract and is not completion of this feature.

## Semantics, paint and focus

Submit each valid enabled-neighbor occurrence once as a button, using the existing localized full-date naming, selection details and root parent. Keep CUI's localized names; frozen ISO-only names remain a documented difference. Disabled neighbors are submitted disabled. Outside-year invalid cells stay empty. Paint real day numbers with muted foreground for unselected neighboring dates; selected/range paint keeps the existing CUI primary/on-primary treatment, disabled foreground remains muted, and today uses the newly implemented precedence. This does not attempt arbitrary GPUI item refinement.

Exactly one day tab stop belongs to the civil cursor: prefer its in-month canonical occurrence. Neighboring occurrences are excluded from sequential Tab, while pointer and AT activation remain available. Keyboard date movement continues using the existing civil-date reducer and rebases the displayed month when necessary, then focuses the canonical day ID. This is CUI's explicit keyboard behavior, not an inferred frozen keybinding. No keyboard dispatch architecture change is needed.

On pointer/AT activation of an eligible neighbor, apply the existing CUI single/range selection reducer once. If the chosen date is within another displayed month, retain the displayed window and focus that canonical occurrence. If it lies outside the displayed window, rebase the window to that date's month and focus the canonical occurrence. This makes activation visible and avoids focus pointing at a removed neighbor. This recentering policy is compatible with CUI's existing endpoint-swapping range model; it deliberately differs from frozen `apply_date` always centering on range start. Do not change the legacy range model as a side effect.

For split reducer/submission, the helper must identify the same dates and IDs in both paths. The reducer matches queued targets against the original displayed-grid snapshot for that outer call, preserving existing queued-input ordering while applying live min/max/disabled policy on every activation. A preceding navigation event must not reinterpret an old occurrence ID as another date. A stale occurrence absent from the snapshot is ignored. No synthetic event insertion, capture/bubble changes or retained hit-test map is required.

## Events and lifetime limits

Navigation alone remains `.navigated`; valid neighbor activation has existing `.activated` and actual final selection `.changed` behavior. No CHANGE event for focus or window rebasing alone. Existing typed observer wrappers do not accept presentation options: do not claim this feature reaches those wrappers. A separately approved `_ex` observer wrapper would call the extended operation once and preserve the existing final-applied CHANGE contract, borrowed user data and outer-return timing. Frozen Selected multiplicity is still a distinct contract and is not supplied by CHANGE.

Cell-info records are copied values. Presentation options contain no retained callback or allocation. Locale strings, disabled predicate/user resources, state and selection retain existing borrowed submission/frame-consumption requirements. Predicates must remain pure with respect to active UI. Rejecting or omitting a neighbor must not retain pointers to temporary grid arrays. Hosts may omit the entire widget after frame consumption without additional cleanup.

## Required focused verification

Use the existing private Release fixture and Xvfb test wrapper, strict C11 warnings and compiling guide. Add actual software-pixel and semantic evidence, not only helper tests:

- Single, two and three months: exactly one semantic/layout ID per valid cell occurrence; same date has distinct neighbor IDs but a stable canonical ID. Different week starts and leap months.
- Mouse, Enter/Space on a programmatically focused occurrence, and queued AT actions select the correct date; ordinary sequential Tab still uses only one canonical cursor. Validate supported focus behavior rather than declaring neighbors inaccessible to keys.
- Neighbor→header and navigation→neighbor queued sequences preserve original target identity; repeated date occurrences cannot redirect selection. Disabled policy changes after prior semantics reject activation live and recover after policy removal.
- Dates before minimum/after maximum, all dates disabled, year 1/9999, panel overflow and invalid helper inputs remain bounded and safe.
- Selected endpoints/interior ranges, unselected muted neighbors, disabled selected neighbors, today neighbors and light/dark themes have the documented actual draw colors/pixels.
- Zero opt-in is byte-equivalent in observable draw/semantic content and software pixels to legacy; no new callbacks or selection changes from merely displaying neighbors. Omission/reappearance retains caller state and consumes no borrowed grid storage later.

A focused headless actual-WASM test follows root export registration; no broad browser gate or source-status promotion from native tests alone.

## Ownership request and review boundary

Proposed production ownership: new helper declarations and presentation opt-in in `include/cui/cui_calendar_presentation.h`; Calendar-only helper/target-resolution/reduction/submission regions in `src/base/cui_picker.c`; Calendar-only day-cell paint in `src/components/cui_picker.c`; focused tests/worker fixture and `docs/CALENDAR_PRESENTATION.md`. Preserve ColorPicker, TimeField, legacy DatePicker and legacy public picker header. DatePicker opt-ins are a separate extension because its current composition passes zero presentation options. Canonical registration, bindings, generated artifacts and all 78 parity entries remain root-owned.

Root approval is required before implementing this proposal. Completion would address selectable neighboring cells under the stated CUI bounds, not full Calendar source style, locale, event or unrestricted-month parity.
