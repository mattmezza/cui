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

## Header-level pure-value proposal (review gate)

This section supersedes the tentative token-safe truncation suggestion above.
It is a proposed header, **not an implemented/exported API**. Fixed-width fields
avoid enum-width assumptions in records; enum constants define their vocabulary.
Include `cui_picker.h` for existing `cui_time`/`cui_str` and standard integer types.

```c
#define CUI_TIME_TEXT_VERSION_V1 UINT32_C(1)
#define CUI_TIME_TOKEN_MAX_BYTES_V1 16u

typedef enum {
    CUI_TIME_TEXT_OK = 0,
    CUI_TIME_TEXT_INVALID_ARGUMENT = 1,
    CUI_TIME_TEXT_INVALID_CONFIG = 2,
    CUI_TIME_TEXT_INVALID_SYNTAX = 3,
    CUI_TIME_TEXT_INVALID_VALUE = 4,
    CUI_TIME_TEXT_OUT_OF_RANGE = 5,
    CUI_TIME_TEXT_BUFFER_TOO_SMALL = 6
} cui_time_text_status_v1;
typedef enum {
    CUI_TIME_TEXT_H23 = 0, CUI_TIME_TEXT_H12 = 1
} cui_time_text_cycle_v1;
typedef enum {
    CUI_TIME_TEXT_MINUTE = 0, CUI_TIME_TEXT_SECOND = 1
} cui_time_text_precision_v1;
enum {
    CUI_TIME_TEXT_HAS_MINIMUM_V1 = 1u << 0,
    CUI_TIME_TEXT_HAS_MAXIMUM_V1 = 1u << 1
};
typedef struct {
    uint32_t struct_size, version;
    uint32_t reserved[2];
    cui_str digits[10];
    cui_str separator;
    cui_str period_separator;
    cui_str am, pm;
} cui_time_locale_v1;
typedef struct {
    uint32_t struct_size, version;
    uint32_t cycle, precision;
    uint32_t flags;
    uint32_t reserved[3];
    cui_time minimum, maximum;
    const cui_time_locale_v1 *locale;
} cui_time_text_options_v1;

cui_time_text_status_v1 cui_time_locale_init_v1(cui_time_locale_v1 *out);
cui_time_text_status_v1 cui_time_text_options_init_v1(cui_time_text_options_v1 *out);
cui_time_text_status_v1 cui_time_validate_v1(
    cui_time value, const cui_time_text_options_v1 *options);
cui_time_text_status_v1 cui_time_parse_v1(
    cui_str text, const cui_time_text_options_v1 *options, cui_time *out);
cui_time_text_status_v1 cui_time_format_v1(
    cui_time value, const cui_time_text_options_v1 *options,
    char *output, size_t capacity, size_t *required_bytes);
```

`struct_size == sizeof(record)` and version1 are required exactly; no prefix
acceptance or implicit zero-size initialization. Reserved fields and unknown flag
bits must be zero; unknown cycle/precision is invalid configuration. No public
old record grows. Under the supported ordinary C11 ABI with 32-bit unsigned and
`sizeof(cui_time)==12`, locale size is240 on LP64 and128 on ILP32/wasm32;
options size64 on LP64 and60 on ILP32/wasm32. These are expected ABI receipts,
not compiler evidence yet: implementation must prove offsets/sizes on actual
targets before publishing. There is no pointer-free wire ABI claim for these
borrowed C records.

Initialization accepts a nonnull writable output and writes a complete record:
ASCII digits, colon, one ASCII space, uppercase AM/PM; options H23/minute,
flags0, bounds zero, locale NULL. The default locale strings have static lifetime;
copying their descriptors is safe. NULL locale selects exactly this immutable
ASCII default. NULL options is invalid argument (use initializer). No mutable
locale singleton, OS locale, registry or allocation. Semantic names are deferred
to the future editor record; they are not unused fields in this pure API.

Every locale token has length1..16 **bytes**, nonnull data, well-formed shortest
UTF-8 scalar encoding, no U+0000/embedded NUL, surrogate or out-of-range scalar.
Tokens may contain multiple scalars. All14 tokens must be pairwise prefix-free:
no equal tokens, and neither token may be the byte prefix of another. This
intentionally rejects ambiguous grammars, even where a position-sensitive parser
could disambiguate. Tokens may otherwise contain spaces/punctuation; there is no
normalization, case folding, trimming or fallback. Period spacing is exactly the
`period_separator` token (default one space), not optional whitespace. Locale
configuration is validated completely even in H23/minute mode, so a later mode
change cannot activate unchecked token data. Input text itself must be valid
UTF-8 without NUL; invalid encoding is INVALID_SYNTAX.

The complete grammars are:

| Cycle/precision | Exact token sequence |
| --- | --- |
| H23/minute | D D separator D D |
| H23/second | D D separator D D separator D D |
| H12/minute | D D separator D D period_separator (am or pm) |
| H12/second | D D separator D D separator D D period_separator (am or pm) |

D is one of the ten configured digit tokens, always exactly two digits per
numeric segment. H23 accepts00..23; H12 accepts01..12 and converts12AM→0,
12PM→12, otherPM→hour+12. Minutes/seconds00..59. No ASCII alternatives when a
custom digit set is supplied, lowercase alternatives, leading/trailing text,
separator navigation or partial parsing. A token-structurally correct `24:00`
is INVALID_VALUE, whereas `9:00` is INVALID_SYNTAX. Parsing minute precision
produces second0. No time zone/date/leap-second/AMPM heuristic.

All value validation first requires structurally valid full `cui_time`.
Minute precision additionally requires second0: format/validate do **not** hide
or truncate a nonzero second. Active bounds must be structurally valid and also
second0 in minute mode; inactive bounds are ignored entirely. Missing minimum
means00:00:00; missing maximum means23:59:00 in minute mode or23:59:59 in second
mode. Inclusive comparison uses hour/minute/second arithmetic, not locale text.
An active minimum greater than active maximum is INVALID_CONFIG; no overnight
wrap. A structurally valid, precision-compatible value beyond bounds is
OUT_OF_RANGE. Formatting applies the same domain checks as parsing/validation.
Owner-only invalid appearance remains separate, not a parser option.

Failure precedence: invalid mandatory pointers/illegal buffer argument pair →
INVALID_ARGUMENT; options/locale/domain configuration → INVALID_CONFIG; parse
encoding/grammar → INVALID_SYNTAX; numeric time/precision → INVALID_VALUE;
bounds → OUT_OF_RANGE; finally format capacity → BUFFER_TOO_SMALL. No error is
stored in a Core context; helpers have no UI dependency.

**Atomic output convention:** parse assigns all three output fields only on OK.
Format never partially writes or clears the output. `required_bytes` is mandatory
and excludes the terminator. On successful validation/format sizing it is assigned
on OK or BUFFER_TOO_SMALL only; all other failures leave it unchanged. A query
uses `output=NULL, capacity=0`, returns OK with required length, writes no text.
A nonnull output with capacity0 returns BUFFER_TOO_SMALL and reports length.
NULL output with nonzero capacity is INVALID_ARGUMENT. Capacity exactly equal
to length is insufficient: success requires length+1 and writes the entire UTF-8
text plus NUL; excess bytes remain untouched. This deliberately differs from the
legacy truncating formatter and avoids malformed/truncated localized labels.
Maximum text length is160bytes (H12/second: six digits, two separators,
one period separator and one period token, each at most16bytes); length+1 cannot overflow.

All input records and token/text storage are borrowed, immutable for the whole
call; no pointers are retained. Writable output ranges (`out`, formatted buffer,
required length) must not overlap each other, any input record, text, or token
storage. This is a documented caller precondition, not a promise to detect
arbitrary C pointer aliasing. No in-place parse/format support. Independent calls
are thread-safe when callers obey these storage rules. Initializers likewise
require writable valid storage; no read of prior output bytes.

### Contract-specific tests before production acceptance

In addition to the matrix above: verify exact size/offset receipts, every enum/
reserved/size/version rejection and failure precedence; all14 token pairs and
prefix directions, malformed UTF-8/overlong/surrogate/out-of-range/NUL, 16/17byte
boundaries and complete custom token grammars. Byte-snapshot outputs before each
rejection (not struct-copy padding comparisons). Test query/NULL pairs,
capacity0/length/length+1 with canaries, required-length unchanged on invalid
value/config, and no-write BUFFER_TOO_SMALL. Exhaustive valid second-of-day
roundtrips use independent arithmetic and checked literal locale vectors rather
than roundtrip alone. Preserve old strict parser and truncation assertions
unchanged. This does not close editor, Styled, IME, accessibility or binding gaps.
