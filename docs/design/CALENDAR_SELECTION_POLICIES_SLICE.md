# Calendar selection and view policies

Design checkpoint only; no proposed declarations or behavior below are implemented. CUI authority is verified integration `9628efc3`. Frozen source authority is `87d10ae5e1299d1be18670a37570c5402f952e34`, inspected locally without moving-main assumptions. Public option names describe behavior and contain no upstream branding.

## Exact source and baseline behavior

Frozen `crates/base/src/calendar.rs` SHA-256: `23f747d81667c1f68a54ca827863d556e167fe500a25eec4049d1c90a7d3cd6e`.

| Decision | Frozen implementation | CUI baseline |
| --- | --- | --- |
| Earlier second endpoint | `select_date:237–257` completes a pending range only when value >= start; otherwise starts a new pending range at value. A completed range's next activation also starts a new pending range. | `cui_picker_select` uses `state.selecting_end`; a second endpoint is accepted and endpoints are swapped if reversed. Next activation after completion starts another range. |
| Authoritative phase | Frozen Date's optional endpoints determine pending/completed state. No separate pending flag. | Caller-owned `cui_calendar_state.selecting_end` controls whether valid start is treated as pending, even if externally supplied endpoints do not match that flag. |
| Year selection | `select_year:385–388` changes current year and returns directly to Day, retaining current month. | Accepted year selection opens Months, then accepted month selection opens Days. |
| Recentering | `apply_date:222–234` stores accepted date and sets current year/month from its start. `select_date` calls it on each accepted activation, including incomplete range starts. | Legacy day selection does not itself always recenter. Neighbor presentation focuses the activated canonical day and recenters only when outside the currently displayed window; keyboard navigation has its separate visible-window rules. |
| Endpoint validation | `Matcher::is_match:118–124` tests both completed range endpoints; `apply_date` rejects the whole proposed selection if either is disabled. `select_date` first checks the newly activated date too. | The live activation check tests the newly activated date; an earlier accepted start can remain after policy changes. Interior range dates are not individually constrained. |
| Events | `activate_date:261–267` emits Selected for a complete accepted selection, including repeated complete activation; incomplete changes still notify state. | Aggregate `.changed` compares final endpoint values, `.activated` records accepted activation, and existing CHANGE wrappers notify zero/one actual final change at outer return. These are distinct contracts. |

Existing evidence: `tests/unit/test_picker.c` tests endpoint sorting, queued day keys, year→month flow and disabled/date bounds; `tests/unit/test_picker_events.c:range_and_chooser` proves sorted reverse range plus silent year/month navigation; `tests/unit/test_calendar_presentation.c:neighbor_routing/neighbor_edges_and_space` proves original-grid pointer/key/AT multiplicity, earlier-end swapping, canonical focus and current recenter behavior. These tests must stay as zero-policy regressions. This checkpoint inspects them; it does not rerun runtime suites.

## Recommended additive API boundary

Use a **new** `include/cui/cui_calendar_behavior.h` and value-only options record. Do not resize the presentation record that now has verified integration/distribution and matching-header/library requirements. Keep existing Calendar functions unchanged and delegate new functions to one private common reducer. Proposed API only:

```c
typedef enum {
    CUI_CALENDAR_RANGE_LEGACY = 0,
    CUI_CALENDAR_RANGE_RESTART_EARLIER
} cui_calendar_range_transition;
typedef enum {
    CUI_CALENDAR_YEAR_OPEN_MONTHS = 0,
    CUI_CALENDAR_YEAR_OPEN_DAYS
} cui_calendar_year_destination;
typedef enum {
    CUI_CALENDAR_RECENTER_LEGACY = 0,
    CUI_CALENDAR_RECENTER_ACTIVATED_DATE,
    CUI_CALENDAR_RECENTER_SELECTION_START
} cui_calendar_recenter_policy;
typedef struct {
    cui_calendar_range_transition range_transition;
    cui_calendar_year_destination year_destination;
    cui_calendar_recenter_policy recenter;
} cui_calendar_behavior_options;

cui_calendar_result cui_calendar_reduce_with_behavior(cui_context *, cui_id,
    cui_calendar_state *, cui_date_selection *, cui_calendar_options,
    cui_calendar_presentation_options, cui_calendar_behavior_options);
cui_calendar_result cui_base_calendar_with_behavior(cui_context *, cui_id,
    cui_calendar_state *, cui_date_selection *, cui_calendar_options,
    cui_calendar_presentation_options, cui_calendar_behavior_options);
cui_calendar_result cui_calendar_with_behavior(cui_context *, cui_id,
    const cui_theme *, cui_calendar_state *, cui_date_selection *,
    cui_calendar_options, cui_calendar_presentation_options,
    cui_calendar_behavior_options);
```

No submit-only behavior variant is needed: `cui_calendar_submit_ex` submits the already reduced state and selection, using the same presentation options. Policies affect reduction, not stored callbacks or styling. Keep public declarations portable C11 and C++ compatible, without platform/Clay types. Zero options are exactly the current reducer, draw and semantic path; an explicit activated-date policy cannot substitute for zero because current legacy behavior conditionally retains a multimonth window.

Normalize each unknown enum value independently to its zero legacy value, preserving other valid fields. Document and test this fallback for negative casts and large values. Alternative: reject the complete operation on any invalid enum, with an explicit validity result. That is stronger validation but needs another result type and risks unrelated maintenance/focus suppression; independent fallback fits the existing forgiving Calendar options better.

Alternative API: append fields to `cui_calendar_presentation_options`. It is fewer function names but makes behavioral policy look like paint configuration and changes that public record's ABI. A versioned record/size-tagged API could support future fields, but adds machinery to a bounded three-policy extension. Booleans are representable but give less readable call sites and no distinct three-state recenter choice. Prefer the independent typed record above.

## Range transition contract

`RANGE_LEGACY` preserves the existing flag-based transition exactly. In range mode, `RANGE_RESTART_EARLIER` treats the valid endpoint shape as authoritative **on activation**, so supplied partial ranges can reproduce the source transition. It does not silently rewrite caller selection or phase merely because an option changes or another frame is rendered.

| Before accepted day activation | Restart-earlier result |
| --- | --- |
| No valid start, with or without end | start = activated date; end = zero; selecting_end = true |
| Valid start and no valid end, activated < start | start = activated date; end = zero; selecting_end = true |
| Valid start and no valid end, activated >= start | keep start; end = activated date; selecting_end = false |
| Valid completed endpoints | start = activated date; end = zero; selecting_end = true |

Equal start/end completes a one-day range. Invalid civil endpoints are treated as absent for transition inference; arbitrary invalid dates are a CUI input concern absent from the frozen typed date representation. When `range=false`, the range policy has no effect and existing single selection remains. Existing bound/predicate checks apply before acceptance. Restart is an applied incomplete selection: `.activated=true` and `.changed` reflects actual final endpoints, not completion alone. Restart-mode completion changes the absent end into a valid date even when both endpoints equal. Zero legacy mode still preserves its phase-only/same-value edge cases with inconsistent caller flags; do not manufacture CHANGE for phase alone.

This full transition mode intentionally includes endpoint-shape inference. A narrower policy that changes only the earlier comparison while still consulting `selecting_end` is an alternative, but cannot reproduce a supplied pending range without extra host synchronization. If root chooses the narrower alternative, expose a separate explicit phase policy later and document that limitation instead of claiming the source transition is covered.

### Additional endpoint-validation gap

The three requested policies do not alone reproduce source behavior when a pending start becomes disabled before completion. Do not hide that difference inside range restart or recentering. A separately approved fourth field can use:

```c
/* Optional addition for review; not required to ship the three-policy slice. */
typedef enum {
    CUI_CALENDAR_VALIDATE_ACTIVATED_DATE = 0,
    CUI_CALENDAR_VALIDATE_RANGE_ENDPOINTS
} cui_calendar_range_validation;
```

With explicit endpoint validation, build a candidate selection in local values, recheck both endpoints for a completed candidate using current min/max/predicate policy, and commit selection plus phase only if accepted. Incomplete candidates validate their activated start. Rejection preserves selection/phase and reports no accepted activation; previously applied cursor navigation can remain. No interval-wide scan: interior disabled dates remain allowed. Zero retains existing live activated-date-only validation. Include this policy only after review; choose whether to include its field before freezing the new record ABI. If approved later, use a versioned or separately extended record rather than silently resizing an installed record. Until then the remaining gap must stay documented and implementable, not silently called complete.

## Year destination and focus

`YEAR_OPEN_MONTHS` preserves year→month behavior. `YEAR_OPEN_DAYS` affects only accepted year-item activation, not highlight traversal, page navigation, month-header toggling or external state changes. Keep the displayed civil month, change its year, and return to Days without modifying selection. Clamp the previous civil cursor's day to the valid day count in that month/year. Prefer that enabled date; otherwise scan at most that month's 31 dates for the first enabled date. If none is enabled, keep the displayed month and a valid civil cursor, but focus the year header rather than a disabled/missing day. Do not jump to another month merely because it contains an enabled date: the frozen year selection preserves month. The existing year-enabled predicate still rejects years with no enabled date anywhere.

This explicit CUI focus rule makes the new path usable; frozen Calendar does not define a roving day cursor/key traversal contract to copy. Accepted year activation reports `.navigated=true`, `.activated=false`, `.changed=false`; selection observers stay silent. Test leap-day clamping, current-month-all-disabled versus whole-year-disabled, bounds, years 1/9999 and header focus restoration.

## Recentering and cursor policy

Apply recenter policy only after accepted **day selection**, including a new incomplete range start. Header navigation and external selection assignments do not implicitly recenter.

- `RECENTER_LEGACY`: exact current behavior, including conditional neighbor window retention and legacy keyboard navigation. No change merely because the new function is used.
- `RECENTER_ACTIVATED_DATE`: always put the activated date's month first, even if it was already in panel two/three; focus its enabled canonical day.
- `RECENTER_SELECTION_START`: always put the resulting selection start's month first. In single mode the resulting start is the activated date. For ranges this uses the committed start after restart/swap, not the previous start and not necessarily the activated endpoint.

For explicit selection-start recentering, make the civil cursor agree with the visible anchor and focus the canonical start when enabled. If an old retained start is now disabled under activated-date-only validation, keep its month visible and choose the first enabled visible day in a bounded ≤93-date search; if none, focus the year header and retain a valid civil cursor. This avoids the existing tail-normalization path undoing the requested recenter, and avoids focus pointing to an endpoint outside the displayed window. A subsequent queued arrow key may then navigate using the existing updated cursor rules. No global key listener or forced focus event injection.

Recenter reports `.navigated` only when it actually changes the displayed month or cursor/focus destination; acceptance still sets `.activated`. `.changed` depends only on final endpoint values. Preserve the existing year-edge bound for displayed panels and never create dates beyond 1..9999. Selected/disabled/today/neighbor paint precedence remains unchanged.

## Ordering and callback compatibility cases

Keep the outer call's original displayed-grid occurrence mapping. Each event applies live policy to the updated selection/phase, with no deduplication of actual activations and no reinterpretation of a previous cell index after recentering. Calendar options/behavior are copied values for the call; predicates remain borrowed and pure. Inactive/disabled/policy-rejected events cannot apply a transition. Do not change core capture, focus-event preprocessing or dispatch architecture.

Required cases before production completion:

1. Pending Feb 20 → activate Feb 10: zero sorts to Feb 10–20 and completes; restart produces pending Feb 10. A following Feb 15 in the same queue completes Feb 10–15. Reverse-order and equal-endpoint cases use actual pointer, Enter/Space and AT routing.
2. Supplied start + no end with selecting_end=false, and supplied completed endpoints with selecting_end=true: zero preserves flag semantics; restart mode follows the documented endpoint shape. Mode changes alone emit nothing and change no endpoint.
3. Two/three visible months with endpoint in another panel: legacy retains the window, activated-date recenters that month, selection-start recenters committed start. Neighbor→header and navigation→neighbor retain original target identity. Repeated activation still runs transitions, rather than collapsing to one event.
4. Year activation with direct-days policy preserves selection and month; returning to days does not consume a second synthetic activation. Queued events targeting old year/month cells are handled only by the existing live view rules, never retargeted as day clicks.
5. Live start/policy changes between earlier frames and completion; rejected newly activated date; optional complete-endpoint validation; no enabled anchor day; widget disabled/inert/passive; omission/reappearance; boundary years and invalid enum fallback.
6. Zero-policy observable draw/semantic/software-pixel equivalence, Base-only and split reduction/submission, plus existing picker and event suites unchanged. No artificial tests that only mirror helper arithmetic.

Existing `cui_picker_events.h` wrappers invoke legacy functions once and notify final applied endpoint changes zero/one times at outer return. They do not take the proposed policy or presentation options. Do not silently change them or claim a bridge. If separately approved, additive extended observer counterparts can reuse `cui_date_selection_handler` and borrowed `user`, call the new operation once, and retain UNKNOWN metadata/no fabricated input index and outer-return timing. Observe incomplete restart changes; silence same-value/phase-only changes, year navigation and recentering. Do not invoke observers per internal transition or reinterpret source Selected as CHANGE. Existing callback mutation/lifetime restrictions and teardown-after-frame-consumption remain.

## Lifetimes, bindings and production boundary

The new record and payloads contain copied portable values; no retained callback or allocation is introduced. Caller owns state and selection. Existing borrowed locale strings and disabled predicate/user resources must survive their documented submission/frame-consumption intervals. Policy implementation must not retain temporary candidate or grid pointers or call application code from painting.

Language builders currently do not expose presentation options, and this proposal adds no bridge marshalling. DatePicker composition and its close-on-completion phase behavior remain separate: applying range restart there eventually needs an explicit DatePicker extension and tests, not reuse of a Calendar-only API name. Public C functions do not imply binding coverage or browser verification.

Suggested reviewable production slices: (1) common bounded behavior record and range transition, (2) year destination plus explicit recenter/focus with combination tests, (3) optional endpoint validation and extended observer wrappers only if separately approved. All share the same private Calendar reducer; no alternate dispatcher or duplicated public implementation. Ownership needed: new header, Calendar-only private Base seams, new focused tests/worker fixture and consumer guide; canonical registration/dist/SDK/metadata/skill refresh follow root approval. Preserve all 78 entries and keep source-specific incomplete items explicit until tested. No production edits are authorized by this checkpoint.
