# Input Number formatting

`CUI_EDIT_FORMAT_NUMBER` is an opt-in single-line, single-selection checked-edit formatter. Zero initialization keeps existing editing behavior. Styled `cui_input_ex` and Base Input forward `.format` and caller-owned `.format_workspace`; no locale, floating-point parser, extra dependency or per-edit allocation is required.

```c
char formatting[256];
cui_input_options options = {
    .format = {.kind = CUI_EDIT_FORMAT_NUMBER,
               .has_separator = true, .separator = ',',
               .fraction_limited = true, .fraction_digits = 2},
    .format_workspace = {formatting, sizeof formatting},
    .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
};
```

The workspace must hold the final UTF-8 value and terminator and remain disjoint from the model, payload, composition buffers and declared presentation resources. It is borrowed only during the synchronous call. Invalid configurations and insufficient capacity fail before predicates or document/history/selection changes. Presentation prepares the final grouped value before acceptance callbacks, including masked display resources; failure preserves logical state and prior committed presentation. Scratch and unused capacity tails are unspecified.

The implementation follows pinned gpui-component 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/base/src/input/base/mask_pattern.rs`: validity lines 189–245, formatting 286–340, unmasking 375–393, numeric normalization 428–458; `input/state.rs` normalization orders numeric conversion before single-line line-break removal.

A configured separator is removed before formatting. Integer scalars group from the right in threes, with the rightmost sign moved to the front. Fraction length truncates only when a separator is configured; zeros are never padded. Without a separator, formatting is identity even with a fraction limit. Empty text, `+`, `-`, `.`, `-.` and `.5` are valid intermediate values. Multiple dots, misplaced integer signs and other characters are invalid, except that configured separators are permitted in integer/fraction parts. This deliberately preserves source quirks, including special separators such as `.` or `+`, rather than parsing a numeric value.

`cui_edit_format_valid`, `cui_edit_format_text`, `cui_edit_format_placeholder` and `cui_edit_unformat_text` accept the Number descriptor. Its placeholder is empty. Unformatting removes configured separators and, if a dot remains, trailing ASCII zeros: `1,234.00` becomes `1234.`. Pure formatting does not validate automatically, allowing the source's invalid-old-value healing policy. The checked engine normalizes the candidate, prepares the final presentation, calls the user predicate on the unformatted candidate, then checks Number validity. If that fails but the previous value was already invalid, the edit can heal it under the existing acceptance contract. Empty candidates bypass predicates. Replay restores recorded values without formatting or acceptance replay; presentation still prepares replayed values.

Fullwidth digits, plus, minus, dot and comma convert to ASCII; U+2212 converts to `-`, and U+3002 to `.`. Other Unicode stays unchanged. `cui_edit_normalize_number_text` performs this conversion for explicit seed setup, optionally removing CR/LF; `cui_edit_number_offset` maps an original UTF-8 scalar-boundary byte endpoint to the normalized byte endpoint. Pure helper output descriptors and buffers are unchanged on errors, and input/output aliasing is rejected. A seed is not automatically normalized, formatted or validated when creating a model.

IME UPDATE applies numeric normalization and optional CR/LF removal but keeps preedit unformatted and committed text unchanged. Selection endpoints are mapped from original host UTF-8 bytes. COMMIT formats once and creates one history edit; CANCEL restores the committed view. The original preedit payload must fit the configured preedit storage before shrinking normalization: this is a bounded ownership rule, not a promise that normalized size alone controls capacity. The final grouped value must fit both model capacity and formatting workspace. Existing source byte-based caret adjustment is retained and snapped to valid UTF-8 boundaries.

CUI differences are explicit: embedded NUL remains unsupported by the editing engine, so an enabled NUL separator is rejected; checked single-line formatting also rejects CR/LF separators. Disabled separator requires `separator == 0`; unlimited fraction requires `fraction_digits == 0`. Single-line CR/LF removal remains opt-in rather than changing legacy controls. Multiline models, multiple selections and inline tokens reject enabled formatting atomically. Raw engine APIs are unchanged. This slice does not implement regex validation or all source content-type builders, and does not claim complete Number-mask or Input parity.

Coverage is in `test_input_number_engine.c`, `test_input_number_control.c` and `test_input_number_native.c`: source intermediate/grouping/unmask cases, UTF-8 endpoint conversion, alias/config/capacity failures, callback order/reentry, invalid-old healing, actual styled paste/IME/semantic state/history, fixed bullet-storage failure and actual-font bullet pixels/factory rollback. Native font test accepts argv font or `CUI_TEST_FONT`, returning 77 when unavailable. Integrators must register these new tests; generated distributions require regeneration after the appended public descriptor fields.

For a registered native build, the engine and control tests link only `cui`; the font test links `cui_text`, `cui_bidi` and `cui`, and uses the optional native text dependencies already pinned by the project. All three compile with `-std=c11 -UNDEBUG -Wall -Wextra -Wpedantic -Werror`. The font test must have CTest skip return code 77. A WASI engine probe uses the engine test plus `cui_edit.c`, `cui.c` and `cui_effects.c`; it requires no filesystem, environment or platform services.
