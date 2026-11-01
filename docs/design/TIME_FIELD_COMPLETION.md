# TimeField completion checkpoint

Status: source audit and proposal only; no production change or completion claim.
Baseline: CUI `332fdaa8897c055fd4d150ca147c6e903dcc543d`.

## Frozen authority

All upstream paths below are relative to
<https://github.com/longbridge/gpui-kit/tree/87d10ae5e1299d1be18670a37570c5402f952e34>.
Only these four primary files were fetched; no dependency checkout was installed.

| File | SHA256 | Relevant lines |
| --- | --- | --- |
| `website/component/time-field.md` | `241e3e4da93bf5b091020a857b36ed35bc483c0f11231d382e1529b88d634363` | 18–103: clocks, seconds, invalid owner styling, keyboard |
| `website/base/primitives/time-field.md` | `d1b405eedb0e24c2c647adc1d0ee686c6a39e5832198506aeb96da6dd5ebbd05` | anatomy, state/events, accessibility |
| `crates/base/src/time_field.rs` | `b79dcdf3c0d5e828d27388c9ca06116dc9fb408ddc0991e1656036ce3dc06ab9` | 128–289 editor; 400–433 navigation/input; 576–631 semantics/composition; 660–776 oracle tests |
| `crates/component/src/time/time_field.rs` | `edd07604da9a86813e5858e926e88673c64daefb5d6f03925937ea28a9847dc6` | 43–144 options and Styled rendering |

The exact source has **no min/max-time API, locale parser, localized digit input,
typed-separator navigation, or IME editor**. Its labels are ASCII digits, `:`,
`AM`/`PM`; key handling accepts unmodified ASCII digits/a/p. These are genuine
remaining CUI requirements recorded in the ledger, but are extensions rather than
features established by this pinned source. Do not erase them or label them
implemented merely because upstream also omits them.

## Coverage and gaps

| Capability | Existing CUI evidence | Remaining work / source distinction |
| --- | --- | --- |
| Valid time-of-day | `src/base/cui_picker.c:1033`, public `cui_time_valid` | Already present: unsigned hour<24, minute/second<60, including hidden seconds. |
| Strict parse/format | `cui_picker.c:1036–1075`; `tests/unit/test_picker.c:61–70` | Already present: exact ASCII HH:MM[:SS], atomic parse failure, bounded NUL format/required length. Broad ledger `parse/validation/localization:not_started` masks this tested subset. No permissive/locale parsing. |
| Controlled value and precision | `cui_picker.h:104–123`; reducer `cui_picker.c:1094–1191` | Minute appearance retains seconds; source precision/set_time truncates seconds. Preserve old behavior; new route must explicitly define truncation. |
| Hour cycles and segments | reducer, `test_picker.c:337–387` | Present H23/H12, optional seconds/period, persistent drafts. Source H12 Up/Down wraps within half-day; CUI wraps 24 hours. |
| Numeric typing | reducer TEXT branch | Source invalid pair restarts from second digit; single digit auto-advances when no second digit can fit. CUI always waits for two and advances even invalid pair. Existing behavior must stay on legacy route. |
| Navigation and reset | reducer key branch | Left/Right currently cycle; source stops at ends. Backspace/Delete absent. Source Tab traverses internal segments before leaving; CUI only selected segment is a tab stop, so global traversal needs a focused proof before promising equivalence. |
| Owner validation | Styled `src/components/cui_picker.c:413–451` | No invalid visual option. Upstream `.invalid(true)` only changes border; never rejects edits. Keep this separate from min/max rejection policy. |
| Bounds | no TimeField options/helpers | Inclusive min/max validation/edit policy missing; overnight ranges need explicit policy, not accidental lexicographic wrap. |
| Localization | paint hardcodes ASCII/English; submit names Hour/Minute/Second/AM/PM | Locale labels/separators/digits, semantic names and matching parse/format missing. Calendar locale support is unrelated. No OS/global locale inference. |
| Readonly/disabled/inert | legacy reducer and observer tests | Present CUI readonly extension and disabled suppression; preserve model/state repairs and established inactive behavior. |
| Change events | `cui_picker_events.h:89–93`, `docs/TIME_FIELD_EVENTS.md`, `test_time_field_events.c` | Existing C typed previous/value aggregate observer after complete Base/Styled operation; not requests/commit. Foreign scalar CHANGE route exists; typed observer not bound. |
| Appearance | paint helper | Fixed segment width exists. Source additionally has size variants, selected-half-radius, input colors, disabled opacity, focus ring, invalid danger border and tabular figures. Current primary selection fill differs. These remain part of full family coverage. |
| Semantics | `cui_picker.c:1193–1213` | Current group + segment TEXTBOX values/ranges/readonly; source root TimeInput with formatted value and one focus handle. No public Core role expansion proposed here; explicitly document representation difference pending accessibility review. |
| DateTimePicker composition | `cui_date_time_picker.h`; `test_date_time_picker*.c` | Embeds old time state and uses private reducer/submit/paint; cannot silently switch to new precision, typing or event policy. |
| Browser/bindings | gallery and existing scalar TimeField builders | Legacy route only; new locale/bounds/options and typed observer are not exposed. All five languages and actual authored examples remain required. |

## Proposed compatible slices (review required)

Do not grow `cui_time_state` or by-value `cui_time_options`, or change existing
parse/format/reducer/wrapper behavior. Add a dedicated portable
`cui_time_field_v1.h` with independent versioned records and new entrypoints;
implementation stays in Base/Component, without Core input dispatch changes.
No production names/signatures below are approved yet.

1. **Pure locale and validation values.** Proposed `cui_time_locale_v1` contains
   version/struct_size/reserved, ten borrowed UTF-8 digit tokens, separator,
   AM/PM tokens and four semantic names. Explicit initializer supplies ASCII
   defaults; callers supply locale, not a locale-name registry or environment.
   Tokens must be nonempty valid UTF-8, unambiguous within their grammar, and
   bounded in byte length; exact limits must be fixed in the implementation
   contract. No case folding/Unicode normalization inference. Borrow for the
   synchronous call only; format copies bytes and semantics follow normal Core
   frame-lifetime rules. `cui_time_domain_v1` has independent has_min/has_max,
   inclusive times, and explicit same-day policy: min>max invalid (overnight is
   a future explicit extension). Proposed `cui_time_parse_v1` returns a typed
   syntax/range/configuration result, assigns output only on success, consumes
   the entire input, uses explicit precision/hour cycle and locale, and rejects
   hidden trailing seconds. `cui_time_format_v1` reports required UTF-8 bytes;
   truncation never emits half a UTF-8 token. Pure validation distinguishes
   structurally invalid time from out-of-domain time. No allocation/global state.
2. **Independent editor route.** Versioned state owns draft/selected segment and
   a snapshot of precision/hour cycle needed to reset drafts on mode changes;
   no pointer ownership. Explicit state init/set-value helpers truncate seconds
   only on the new minute route and preserve draft on identical owner echo.
   New Base/Styled options contain old disabled/readonly meanings, domain,
   locale, size and owner-invalid flag; rejected out-of-domain edits preserve
   value and report validation rather than clamp silently. Source typing,
   half-day wrapping and delete/reset are implemented here, not backported to
   legacy. Typed separators advance only on the new route; full pasted formatted
   values must be atomic rather than interpreted digit-by-digit. Tab behavior
   requires tests against actual existing targeted traversal before API promise.
   No generic text-input/IME claim: inline TEXT cannot establish IME composition;
   report this architectural gap separately instead of claiming locale completion.
3. **Styled and observed delivery.** Separate source-sized presentation/invalid
   border/focus/selection work, with real software pixels. Reuse actual theme
   metrics and source sizes; preserve explicit owner-invalid styling independently
   of rejection. New events remain copied aggregate previous/value after complete
   composition, with independent validation outcome; never silently reinterpret
   old CHANGE as proposal/accept. Names and semantics share locale authority.
4. **End-to-end delivery.** Extend DateTimePicker only through additive opt-in,
   then C bridge and Python/JS/TS/Go/Rust options/records with actual UI examples,
   exact callback/lifetime/optional-symbol contracts. Gallery demonstrates the
   new route after tests; old scalar route remains a compatibility fixture.

Recommended next implementation is slice 1, after exact public record/token
limits and formatting failure convention are reviewed. It is independently
verifiable, resolves real parse/validation/localization gaps, and does not require
input architecture. Full TimeField remains partial after that slice.

## Required evidence before completion

- Freeze old Base/Styled and DateTimePicker traces before changing shared private
  helpers: commands/IDs, semantic values/bounds, digit drafts, focus, callbacks,
  net-zero batches, disabled/readonly/inert/layout-only, zero-owner legacy calls.
- Independent table oracle across 00:00:00/23:59:59, noon/midnight, all four
  precision/cycle combinations, valid/invalid pairs (e.g. hour 29), auto-advance,
  segment endpoints, H12 11→12 period preservation, delete source quirks. Source
  clear uses displayed value0 even for H12 (maps to12); preserve/document it.
- Parse/format every valid second-of-day against independent arithmetic; locale
  token tables with multibyte digits/periods, malformed UTF-8, ambiguous/duplicate
  tokens, embedded NUL/lengths, truncated inputs, empty/extra text, capacities
  0/1/exact/short and unchanged outputs on configuration/parse failures.
- Bounds exact inclusivity, hidden seconds, min=max, missing endpoints,
  min>max rejection, programmatic value outside domain versus user edit,
  validation-only invalid appearance and disabled/readonly no mutation.
- Actual original-order multiple events, paste atomicity, owner echo/mode change,
  focus/Tab/Shift-Tab exit, omission/reappearance/modal/inert behavior and callback
  timing/copy lifetime; no duplicated operation or synthesized input metadata.
- Styled first-frame/resize/focus/disabled/invalid pixels and all theme/size paths;
  tests under private Xvfb or windowless software, browsers headless. IME and
  physical AT remain explicitly unsupported until actual host evidence exists.
- New APIs require metadata, usage guide, skill generation/freshness and all78
  preservation; update broad ledger to honest partial coverage, not full closure.

No tests or compiler were run for this documentation-only checkpoint. Source was
read and primary-file hashes compared with the frozen documentation hash.
