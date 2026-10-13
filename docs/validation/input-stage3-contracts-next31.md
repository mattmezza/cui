# Input Stage 3 contract proposal

Read-only audit. CUI base: `3d8aa8b3d763f25dbfec54841e8b20b01051b6af`.
Reference: gpui-component 0.7.1 checkout
`87d10ae5e1299d1be18670a37570c5402f952e34`. No production implementation,
dependency installation, or full Input parity claim accompanies this proposal.
Stage 2 password hiding is already integrated; a formatting mask below changes
the stored value and is a different feature.

## Verified source contracts

All source paths below are relative to that exact reference commit. Implementation
and source tests take precedence over incomplete builder comments. In particular,
the state builder comment calls `9` a digit or dot, but MaskToken::Digit accepts
ASCII digits only. It omits `#`, which the implementation supports.

| Feature | Exact evidence | Contract |
| --- | --- | --- |
| Acceptance chain | `crates/base/src/input/base/state.rs:3534–3553` | Empty succeeds without callbacks. Otherwise custom validator, mask validity, then regex, with short circuiting. |
| Pattern builders | same file `10539–10552` | Receive an already compiled `regex::Regex`; setter changes policy without rewriting or revalidating the existing document, selection, or history. |
| Replacement | same file `3996–4002,4109–4165` | Normalize inserted text, build whole unformatted candidate, reject only if candidate invalid AND previous value valid. Then format accepted candidate; invalid old text remains editable even if the next candidate is invalid. |
| IME update | same file `4220–4285` | Normalize and validate prospective marked text with the same old-valid healing rule; do NOT run formatting during update. |
| IME commit/history | same file `4125–4198` | Format at commit; changed formatting records one atomic whole-document history change with final caret. Replay bypasses normalization, validation, and formatting, restoring exact recorded bytes/selections. |
| Formatting builders | same file `3565–3598` | Set explicit mask flag; pattern placeholder becomes the field placeholder. Number mask has no placeholder. Removing/changing mask does not eagerly rewrite value; no automatic restoration of an earlier placeholder. Explicit None prevents default NumberInput mask. |
| Seed/readback | same file `1241–1249,1271–1273` | Default value normalizes, but does not validate/format. `value` is formatted stored text; `unmask_value` is an explicit extraction operation. Builder order affects numeric seed normalization. |
| Cut | same file `2666–2676` | Copy selected text to clipboard first, then attempt atomic deletion. Rejected deletion can still have copied; clipboard side effects are not transactional document state. Password policy still denies copy/cut before callbacks. |
| Content type | `crates/component/src/input/input.rs:27–96,328–335,689`; `content_type.rs:7–174` | Component hint independent of formatting and masked drawing. Native sync happens only for editable controls; source native implementation is macOS-only. |

The lock pins `regex` 1.13.1, `regex-automata` 0.4.18, and `regex-syntax`
0.8.11. Search matching is intentional: unanchored `abc` accepts `xabcx`;
anchored `^abc$` does not. This follows the actual call to `is_match`, also
specified by the [pinned regex API](https://docs.rs/regex/1.13.1/regex/struct.Regex.html#method.is_match).
A substring matching provider cannot be silently replaced with HTML pattern
whole-value matching, POSIX regex, or a byte-only ASCII `\w` engine.

## Formatting details that must survive translation

`crates/base/src/input/base/mask_pattern.rs:100–410` is authoritative:

- `9`: ASCII digit; `A`: ASCII letter; `#`: ASCII alphanumeric; `*`: one Unicode
  scalar, not one byte or grapheme. Other scalars are literal separators. There
  is no escaping syntax for making these four token characters literal.
- Validation walks tokens and consumes the next input scalar only if it matches
  that token. Unmatched tokens are skipped; validity means all input was consumed.
  Thus partial prefixes and missing separators can be valid. This is not a
  conventional fixed-position mask or a requirement to fill every slot.
- Formatting walks tokens, inserts separators without consuming a different
  scalar, consumes matching literals, and stops at the first unmatched nonliteral
  token. It stops when input is exhausted, so no automatic trailing separators.
  Truncation is source formatting behavior only after accepted/healing input;
  buffer truncation must never masquerade as that behavior.
- Pattern unmask walks token positions, drops literal positions, and retains the
  other scalars. It is not arbitrary punctuation removal. Placeholder retains
  literals and emits `_` for input tokens.
- Number validity allows one dot, ASCII digits, a leading sign, and configured
  separator scalars in either integer or fraction. It does not enforce grouping
  positions or the configured fraction count. `.`, `-.`, and a bare sign are
  valid intermediate values. A second dot or fraction sign is invalid.
- With a separator, number formatting removes it, groups integer scalars from
  the right in threes with sign outside the groups, and truncates the fraction
  to the configured count. Zero fraction count removes the dot/fraction. It does
  not pad missing zeros despite a misleading comment. Without a separator,
  formatting returns text unchanged, including when fraction count is configured.
  Number unmask removes separators and, if a dot exists, trims trailing `0`
  scalars; it may leave a trailing dot (`1.00` becomes `1.`).
- Number-only insertion normalization maps fullwidth digits/plus/minus/dot/comma,
  U+2212 minus, and U+3002 full stop to ASCII. It happens before single-line CR/LF
  removal and validation, including IME update. Original public UTF8 byte endpoints
  must be mapped through both transforms; the reference uses host UTF16 offsets.

The source caret formula after formatting is byte-based:
`min(range.start + saturating_sub(inserted_bytes + formatted_bytes,
candidate_bytes), formatted_bytes)`. Keep this for ordinary ASCII cases. CUI's
public offsets must also remain scalar boundaries for Unicode literals; explicitly
test boundary snapping rather than publishing an interior UTF8 offset.

## Current CUI and smallest complete next slice

`cui_edit_apply_checked` and the composed Base/styled Input already provide
whole-value acceptance, old-invalid healing, bounded preview, replay bypass,
CR/LF normalization, and hard resource preflight. They do not provide a regex
object, format pattern, numeric format normalization, placeholder generation,
or unformatted readback. A user callback can implement a predicate today, but
cannot safely implement post-acceptance document formatting by recursively editing
the same model. Presentation callbacks must remain resource gates, not formatters.

Recommend implementing **pattern-token formatting first**, including the actual
styled reducer/IME/history path and pure seed/readback helpers. Include Number
as a subsequent explicit mode, not a guessed change to Numeric/OTP policies.
Regex and broader content-type mappings remain separately measurable slices.
None of these options become default behavior for legacy zero-initialized CUI
options. Multiline formatting is explicitly unsupported, matching the source
single-line application path; the pure helpers remain usable independently.

Proposed signatures (names are proposals, not existing ABI):

```c
typedef enum {
    CUI_EDIT_FORMAT_NONE = 0,
    CUI_EDIT_FORMAT_PATTERN,
    CUI_EDIT_FORMAT_NUMBER
} cui_edit_format_kind;
typedef struct {
    cui_edit_format_kind kind;
    cui_str pattern;              /* synchronous immutable UTF8 */
    uint32_t separator;            /* 0 means no separator; otherwise valid scalar */
    bool fraction_limited;
    size_t fraction_digits;
} cui_edit_format;
cui_edit_result cui_edit_format_valid(cui_edit_format, cui_str, bool *valid);
cui_edit_result cui_edit_format_text(cui_edit_format, cui_str,
    char *destination, size_t capacity, cui_str *formatted);
cui_edit_result cui_edit_format_placeholder(cui_edit_format,
    char *destination, size_t capacity, cui_str *placeholder);
cui_edit_result cui_edit_unformat_text(cui_edit_format, cui_str,
    char *destination, size_t capacity, cui_str *unformatted);
/* Append to checked request, then Base/styled Input options. */
cui_edit_format format;
cui_edit_workspace format_workspace;
```

No hidden edit-time allocation: caller supplies format workspace large enough for
the final formatted value plus NUL. Model-owned scratch retains the unformatted
candidate while workspace holds formatted output. Check descriptor, pattern,
workspace, output, model/session/payload, and presentation declared spans for
overlap before writes. Pure helper descriptors/output parameters are also disjoint.
Malformed UTF8, enum/scalar/config misuse and size overflow are hard errors;
capacity failure is atomic and distinct from semantic validity false. Fraction
and pattern length limits must be explicit bounded configuration, not silently
clamped. Borrowed configuration lives through the synchronous request only.

For committed edits, build normalized candidate, compute final format in disjoint
workspace, hard-prepare resources for that final value, then run the source
acceptance chain on the original unformatted candidate and old visible value.
Computing formatting is pure and does not invoke predicates. A resource failure
precedes predicates as the existing CUI hard-gate contract requires. If candidate
invalid and old valid, discard staged final resources and preserve logical state.
If accepted, adopt final bytes and final caret as one transaction. When formatting
changes value, use one whole-document history entry; do not first commit candidate
then replace it. UPDATE prepares/validates the unformatted composed view; START,
CANCEL, PRESENT and replay retain their established semantics, with hard resource
preparation for actual presented text. COMMIT prepares formatted final text once;
host rejection still cancels its ended IME session afterward. No callback replay.
Undo/Redo restores saved final bytes and selection while preparing resources, with
no predicate/formatter calls. Setter changes must not retroactively rewrite history.

Single-selection parity is sufficient for formatting's first implementation:
reject multiple selections atomically as unsupported before callbacks/writes,
rather than invent independent field formatting. Existing zero-format multi-cursor
editing remains unchanged. Guard same-model mutations exactly as checked acceptance
does; direct caller-buffer writes/external side effects remain outside protection.

## Regex proposal and dependency boundary

Do not add POSIX/system regex as a dependency: Windows/WASI differ and Rust Unicode
semantics would be lost. A zero-dependency bounded C11 Thompson-NFA implementation
can first cover the actual story `^[a-zA-Z0-9]*$`, ASCII classes, scalar literals,
grouping/alternation, scalar dot, anchors and bounded/simple repetition. It must
explicitly reject unsupported syntax/flags/Unicode classes during compilation.
This is a named portable dialect subset, not full Rust regex 1.13.1 equivalence.
No backtracking or unbounded recursive parser; caller-owned compiled storage and
per-match state scratch, checked compile/state/step limits, no per-edit allocation.

Proposed standalone compile/query surface:

```c
typedef struct cui_input_pattern cui_input_pattern; /* immutable caller storage */
cui_edit_result cui_input_pattern_compile(cui_str expression,
    cui_edit_workspace storage, cui_input_pattern **out);
cui_edit_result cui_input_pattern_match(const cui_input_pattern *, cui_str value,
    cui_edit_workspace scratch, bool *matches);
/* Append separate pattern+scratch fields to checked acceptance/Input options. */
```

Execution errors are hard failures, not `false` predicates: otherwise an exhausted
regex budget could wrongly heal an invalid-old value. Integration order remains
custom validator, mask validity, pattern match; empty bypasses all and replay
bypasses all. Pattern setter merely swaps the immutable configured object. Objects
and scratch spans must be declared for engine alias/reentrancy validation. Full
Unicode regex would require a separately reviewed/pinned parser and Unicode data
receipt/license plan; nothing in this audit installs Rust or adds a host dependency.

## Content-type proposal

Reference enum has 45 variants. Current CUI implements UNSPECIFIED/PASSWORD/
NEW_PASSWORD only. Add remaining purposes to the existing independent Core enum,
appending ordinals, and pass them through finalized owner/generation configuration.
Do not rename component fields or introduce Core-to-component dependency. Keep
hidden/revealed type separate from purpose; content type never filters, formats,
or masks a value. Policy change restarts only the focused host proxy/session and
must preserve document/history. Disabled/readonly stops host configuration.

Public semantic details can carry purpose; dedicated native roles for telephone,
email, URL, DateTime and Birthdate require an explicit semantic mapping extension.
All other source purposes retain generic text role; multiline wins in source role
selection. Password/new-password retain protected-value behavior already implemented.
Browser autocomplete tokens are a host mapping, not a promise of autofill or
validation. Source macOS names are not all standard HTML tokens (`location`,
`address-level1+2`, tracking/flight/date-time, cellular metadata); expose unsupported
capabilities honestly instead of sending invented tokens. SDL can map only its
pinned input-type hints (e.g. email/URL/number), not all 45 autofill identifiers.
No native macOS content-type claim follows from portable metadata alone.

## Acceptance evidence required before production handoff

Port reference cases from `crates/kit/tests/input/constraints.rs:305–390,618–696`
through actual Core+styled Input, not just helpers:

- Pattern `99-99`, paste `1234` => stored/drawn `12-34`, caret5, unformat `1234`;
  following overflow rejected without a Change/Undo entry; one Undo => empty and
  Redo => exact `12-34`/caret. Separator deletion and literal replacement tests.
- `(AA)999-999` omitted and present separators; prefix and doubled separator
  behavior from `mask_pattern.rs` tests; Unicode `9999年99月` and `*999` scalar
  boundaries, emoji/combining clusters, placeholder and unformat behavior.
- Empty always accepted; invalid seeded old text permits healing, including a
  candidate whose formatting truncates as source specifies. Runtime policy swap
  does not rewrite seed; old-invalid predicate chain and exact callback counts.
- Actual IME UPDATE remains unformatted/model unchanged in CUI; COMMIT formats
  once into one Undo step; rejected UPDATE retains prior view, rejected host
  COMMIT cancels; source-vs-CUI provisional document difference remains explicit.
- Numeric mode `12。5` => `12.5`, `。` => `.`, signs/fullwidth symbols/grouping,
  configured fraction with/without separator, zero fraction, Unicode separator,
  normalization endpoint mapping and exact Undo. Do not reuse OTP first-scalar
  rules or silently change Numeric existing acceptance.
- Regex anchored vs unanchored; empty bypass, chain short circuits, old-invalid
  healing, malformed/unsupported pattern atomic compile, exhausted workspace
  hard failure, readonly and replay no matching; source story ASCII expression.
- Forced format capacity and presentation allocator/factory failures preserve
  value, selection, history/coalescing, prior IME view and resource ownership;
  descriptor/data aliases and same-model mutation guarded. Native font and real
  WASM/CDP verify final caret/paint and mode/owner rebind.
- Content purpose does not mutate text; complete enum/role mapping table, unknown
  enum atomic rejection, focused-generation changes, ordinary multiline, readonly/
  disabled/no-focus reset, actual supported browser tokens and SDL hint properties.

Remaining holes include full Unicode regex dialect, numeric mask, 43 additional
purposes and native autofill, source Focus/Blur/Change/PressEnter callback stream,
cursor blink lifecycle, platform-specific shortcuts/character palette, delayed
clipboard generation/revision transaction, formatting inline-token policy, and
source mask-toggle/clear affordances where not yet composed. No full-family status
change is justified by this proposal.

## Baseline validation

Strict configure succeeds. Build from committed `3d8aa8b` fails linking examples:
`cui_components.c` calls `cui_base_button_group_begin/end/item`, but committed
CMake has not registered `src/base/cui_button_group.c`. Root confirmed registration
is intentionally uncommitted during joined/password/shadow integration. No audit
files cause this failure; no global files were edited. CTest was not reached by
the chained build gate, so no passing suite claim is made. Logs:
`/home/matteo/.cache/cui-validation/input-stage3-{config,build}.log`.
Integrator must run its registered warning build/CTest after importing these docs.
