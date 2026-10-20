# Calendar keyboard and localization audit

Design checkpoint only. No API or implementation below is shipped by this document.

## Authority and method

CUI baseline: `19e390c4`. Frozen GPUI source: `87d10ae5e1299d1be18670a37570c5402f952e34` (0.7.1), inspected from the existing local checkout, without network or moving-main assumptions.

| Frozen file | SHA-256 |
| --- | --- |
| `crates/base/src/calendar.rs` | `23f747d81667c1f68a54ca827863d556e167fe500a25eec4049d1c90a7d3cd6e` |
| `crates/component/src/time/calendar.rs` | `842e291959ebec26887599289a3e1b52da24870be785ef415efa11f824321251` |
| `website/component/calendar.md` | `e9a388c6b802b26c63cf44620059546929d3cf8b02403708badbb71fc8262f8b` |

Source locations below refer to that revision. This is source/test inspection, not a new runtime validation. No tests, generated resources, global parity records or production files changed.

## Frozen contract and existing CUI evidence

| Area | Frozen behavior | CUI baseline and consequence |
| --- | --- | --- |
| Keyboard | Base `calendar.rs:635–827` installs click handlers and tracks the state's root focus handle. There are no Calendar-specific key bindings, action handlers or key-down handlers in this module. The comment at 259–261 calls `activate_date` a pointer/keyboard path, but does not implement keyboard traversal. The website introduction promises comprehensive keyboard navigation without a key table. | `src/base/cui_picker.c:235–554` already implements ordered traversal, focus movement and activation. Do not invent an upstream arrow-key contract from the website sentence. |
| Day navigation | No explicit traversal keys in frozen Calendar. | Arrows move 1/7 days; Ctrl/Meta arrows move months/years; PageUp/Down move months (Ctrl/Meta years); Home/End use configured week start. Disabled dates are skipped with a bounded 366-iteration search. Enter/Space use the existing interaction activation path. Multiple queued events use the updated cursor. |
| Picker navigation | Month/year click transitions; independent month and year toggles for a single-month view. Month view disables previous/next; year pages obey configured year range. | CUI has days/years/months, 20-year pages and keyboard grid traversal, Escape back to days, but only the year-header public ID (part 5). Year selection leads to month selection; no independent month-header toggle. |
| Localization | Component `calendar.rs:12–30,89–107` supplies translated month/weekday labels through Base's `label(kind,value)` callback. First weekday defaults Sunday and is separately configurable, not derived from locale. Other values are decimal numbers; navigation is chevrons. | `cui_date_locale` already supplies borrowed month/weekday strings, seven builtin tables (US/GB English, Italian, French, German, Spanish, Japanese), navigation/root/picker labels and numeric date order/separator. Explicit locale normally chooses week start; `first_weekday_override` restores caller control. NULL locale preserves legacy ISO names/week-start behavior. This is an intentional CUI contract, not identical ambient rust-i18n state. |
| Accessible names | Base `with_label` sets aria-label even after decorative children replace text (509–512). Day names override labels to ISO full date (775). Header/month/year names are their labels. | Days already use full dates via `cui_date_format_locale`, with selected/parent semantics and a roving tab stop. Navigation, root and picker labels are localized. CUI's localized full date is useful but differs from frozen ISO day names; preserve defaults and offer an explicit override only if needed. |
| Neighboring dates | All dates returned in week rows are rendered, muted when outside that displayed month, and remain selectable unless the matcher disables them (755–784). IDs include panel offset and week index. | CUI computes 42 cells but deliberately submits/paints only in-month dates; other cells are empty. Existing public day ID depends only on date, so simply rendering duplicates across panels is unsafe. |
| Today | State captures host local date at construction (183–197). Item state marks today (770); Component gives unselected today accent paint. | No supplied today date or today-specific paint. Core should accept a host-supplied civil date, never acquire wall-clock/timezone state. |
| Selection/events | `activate_date` emits Selected for every enabled complete selection, even an already selected date. Earlier second range endpoint restarts the range rather than swapping it (237–267). | Preserve CUI legacy range behavior and `.changed/.activated/.navigated` aggregate. Existing typed CHANGE observers notify actual final endpoint changes once at outer return; they are not frozen Selected events. Do not relabel them as such. |
| Bounds/layout | Source has configurable year range (default today ±50), unrestricted month count (facade clamps effective width count to at least one), item styling and Small/default/Large sizes. | CUI Gregorian years 1–9999, min/max/predicate policies, one-to-three months, custom cell size. These bounds and style limitations remain explicit; this audit does not erase them. |

Frozen Calendar does **not** implement week-number columns or hover range previews. Those may be separate product extensions but cannot be cited as missing frozen Calendar features. CUI's manifest limitation currently groups them with genuine today/neighbor gaps.

## Evidence already present

`include/cui/cui_picker.h:20–83` is the public authority. `tests/unit/test_picker.c` covers Gregorian grids over forty years/all week starts, locale formatting/truncation/fallback, locale week-start override, year/month grids and boundaries, bounded selection, ordered queued keys, Enter/Space activation, roving semantics, custom month and picker labels, and cell-size presentation. Its Base-only build also exercises behavior independently of Styled paint.

`tests/web/test_date_locale_year.py` drives actual WASM with mouse and CDP touch for Calendar and DatePicker in seven locales: twenty-year grid geometry, localized month/weekday paint, actual selected date, localized trigger semantics and Escape transitions (28 cases in the fixture). It is not proof of every keyboard combination. `tests/unit/test_picker_events.c` compares legacy and typed operations, including ordered changes, disabled policy, navigation and observer restrictions. None of these suites was rerun for this documentation-only checkpoint.

The manifest's `keyboard/localized semantics: not_started` conflicts with these APIs/tests and the separate tested ordered-keyboard feature. Root should reconcile it feature by feature after reviewing evidence, without claiming full Calendar parity or removing any of the 78 entries.

## Proposed cohesive implementation

Recommended next production scope: an additive **Calendar presentation options** facade in a new header, delegating to the existing reducer and semantics, with opt-in direct month toggle and supplied-today styling. This yields visible source-backed improvements while keeping the existing keyboard and locale API authoritative. Neighboring-day interaction is a separate slice because it needs occurrence identity and ordered hit-target mapping.

Proposed names/types are illustrative until API review:

```c
typedef struct {
    bool month_toggle;       /* zero preserves year-then-month path */
    cui_date today;          /* invalid/zero means no today decoration */
} cui_calendar_presentation_options;
/* base/styled _ex counterparts take existing options plus this record. */
```

The extra month header needs a new documented part ID, a live eligibility check, selected/open semantics, keyboard activation, and deterministic focus restoration to the current month. It must not reuse the existing year part or change its meaning. Only expose the source's independent toggles for one-month mode; preserve legacy multi-month headers. Today paint must lose to selected paint, retain disabled contrast, and not change selection, cursor, eligibility or event emission. Existing default calls retain exact semantics and pixels. A standalone today paint helper is an alternative if adding an extended submission path would duplicate reducer code; prefer one shared private implementation.

Alternative A (smallest): append opt-in fields to `cui_calendar_options`. It avoids facade proliferation and naturally reaches DatePicker, but changes public struct ABI, canonical bindings and generated layouts; it also touches a shared public header. Alternative B (recommended): a new extension header with `_ex` functions and an independent record leaves legacy struct layout and caller defaults intact. DatePicker support must be an explicit corresponding extension, not silently inferred through unrelated options. Alternative C: caller-controlled state view plus custom Base composition needs no API, but does not supply a reusable Styled month toggle and is not a complete solution.

Do not add a new keyboard enum or dispatcher: retain CUI's portable keys/modifiers and existing reducer ordering. Expand focused tests for modifier/month-end/leap-day/week-start navigation, blocked policies, years 1/9999, multiple keys followed by activation, root versus child focus and AT activation. These should establish the actual CUI contract; they cannot retroactively prove a frozen key binding that is absent.

## Localization extension decision

Current tables already cover the frozen component's month/weekday translation capability for those supplied languages. Adding every frozen translation file is data coverage, not a keyboard architecture change. Applications can supply other languages today. Numeric date order is not a locale engine: arbitrary patterns, non-Gregorian calendars, localized numerals, grammatical month forms, RTL layout and OS locale lookup remain unsupported.

If custom accessible day labels are required, prefer a bounded formatter callback in the extension record, with `void *user`, date plus item kind/context, caller-provided output buffer/capacity, and required-byte-count excluding NUL. Enforce safe termination, UTF-8 boundaries, a documented maximum, overflow/error fallback, and identical semantic/paint decisions where appropriate. A returned borrowed `cui_str` callback is simpler but exposes temporary storage and frame-lifetime hazards; avoid it. A separate accessible-date formatting helper is the smallest alternative but does not customize submitted Calendar names. No callback is necessary merely to finish the existing locale-table coverage.

## Separate neighboring-date slice

Offer explicit opt-in rendering/activation; retain empty cells by default. Define occurrence IDs containing panel/cell plus a helper mapping to civil date, while keeping `cui_calendar_day_id` stable for legacy in-month dates. Decide a unique roving occurrence when a date appears twice, avoid duplicate accessible IDs, and keep keyboard movement based on civil dates rather than panel order. Pointer/AT activation resolves the occurrence to a date through the same bounded policy checks. Test adjacent panels, click versus queued keyboard targets, minimum/maximum and year-edge invalid cells, selected range paint and software pixels. This deserves its own reviewed API checkpoint before production.

## Lifetimes, events and bindings

All proposed records/callbacks are borrowed for the outer submission; do not retain callback/user in state. Existing locale table/strings must remain alive while options are used, and any resources referenced by frame paint/semantics must survive consumption. Formatters and disabled predicates must be pure with respect to active UI/state and cannot recursively submit, free or rebind resources. Host supplies today; no platform or Clay types enter public APIs.

Existing `cui_picker_events.h` wrappers call the legacy operation once and observe final applied selection, UNKNOWN source, no fabricated input index. Navigation, month-view toggles and today decoration must remain silent to CHANGE. Preserve exact callback timing and per-event live policy checks; no capture/bubble/dispatch changes. If a Selected-style activation observer is later needed, design it separately with honest multiplicity/source evidence rather than stretching CHANGE.

The generic bridge currently constructs Calendar options with disabled state and owns Calendar state; it does not expose the complete native locale/policy contract. New C APIs do not imply Python/JS/TS/Go/Rust option support. Root must register sources/tests and separately refresh exports, SDK, metadata and skill resources after integration. Binding marshalling must copy strings or document ownership and must not store ephemeral native callback pointers.

## Proposed verification and approval boundary

Production requires explicit root ownership approval. Suggested first slice: new extension header, Calendar-only private implementation changes, focused native tests and compiling consumer guide. Run strict portable C11 Release builds and relevant Base/Styled/picker-events tests through the private Xvfb wrapper; verify zero-extension legacy draw/semantics equivalence, actual software pixels for today precedence, mouse/key/AT month toggle, disabled and multi-month paths, and bounded lifetimes. Follow with a focused actual-WASM fixture only when root registers exports. No canonical build/metadata edits are part of this audit.

## File ownership needed for production

A new `include/cui/cui_calendar_presentation.h` can carry the additive facade without changing `cui_picker.h`. Implementation still needs Calendar-only private seams in shared `src/base/cui_picker.c` (ordered reduction, header target IDs and semantic submission) and `src/components/cui_picker.c` (header composition and today paint). Root must reserve those regions; new source files alone cannot safely add header activation to the existing reducer. The pure locale formatter remains in the existing Base file and should not be duplicated. Tests can be new `tests/unit/test_calendar_presentation.c` plus a standalone worker fixture and `docs/CALENDAR_PRESENTATION.md`; canonical registration remains root-owned. Existing event wrappers will need a separately reviewed extended counterpart if the new facade is offered with typed observers; do not silently route an extended call through an old wrapper that drops options. Neighboring dates require another explicit ownership grant for those same Calendar-only seams.
