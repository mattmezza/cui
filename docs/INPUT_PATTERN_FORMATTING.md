# Pattern formatting for single-line Input

This opt-in feature formats the stored value. It is independent of password bullet
display, regular-expression matching, numeric grouping, and semantic content type.
Legacy zero options are unchanged. No full Input parity claim follows.

```c
char formatted_workspace[128];
cui_input_options options = {
    .format = {CUI_EDIT_FORMAT_PATTERN, {"99-99", 5}},
    .format_workspace = {formatted_workspace, sizeof formatted_workspace}
};
/* Paste 1234 stores/paints 12-34; the final caret is byte 5. */
cui_input_result result = cui_input_ex(ui, id, &theme, label, edit, &state, options);
```

The appended fields also exist in `cui_base_input_options` and `cui_edit_request`.
Use `cui_edit_apply_checked` for an independent reducer. Direct raw edit functions
have no retained format policy. Pattern/configuration changes never eagerly rewrite
existing text or history. Pure helpers support seed/readback setup:
`cui_edit_format_valid`, `cui_edit_format_text`, `cui_edit_format_placeholder`,
and `cui_edit_unformat_text`. Seed creation itself still does not automatically
validate or format; call the pure helper explicitly if that is the desired policy.

## Source behavior

Pinned reference: `87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/base/src/input/base/mask_pattern.rs` and `state.rs:3534–3553,4109–4198`.
The source's incomplete builder comment is not the token contract:

| Token | Consumes |
| --- | --- |
| `9` | One ASCII digit, not a dot or fullwidth digit |
| `A` | One ASCII letter |
| `#` | One ASCII letter or digit |
| `*` | One Unicode scalar, not one grapheme |
| Other scalar | Literal separator |

Validation scans tokens, skipping mismatches and consuming matching scalars; all
input must be consumed, but not every token must be filled. Formatting inserts
unmatched separators, consumes matching separators, and stops at the first
unmatched input token or exhausted input. It does not append trailing separators.
These source rules are intentionally preserved, including `A9` accepting `1`
while formatting it to empty. There is no token escaping syntax.

Unformat walks token positions and removes literal positions, rather than stripping
all punctuation. Placeholder uses `_` for input tokens and preserves literals.
For `9999年99月`, `202412` formats to `2024年12`; placeholder is `____年__月`.
An empty pattern behaves as NONE. Styled Input uses the generated placeholder
when no explicit nonempty `.placeholder` is supplied; an explicit placeholder wins.
This frame-options rule is distinct from the reference's ordered builder setters.

For each checked committed edit: normalize insertion if requested; build whole
unformatted candidate; compute formatted final text in caller workspace; hard
prepare final presentation resources; then run custom validator followed by token
validity on the original candidate. Empty bypasses both. If invalid, run that same
chain on the old visible value; reject only when old is valid. Thus invalid seeded
text remains editable. Hard capacity/resource errors never use invalid-old healing.
The user predicate is not replayed for formatting. Masked Input prepares only final
bullet geometry; no plaintext geometry fallback is introduced.

IME UPDATE validates/prepares an unformatted composed view and never mutates the
committed document in CUI. COMMIT formats once and creates one Undo transaction.
Rejected UPDATE preserves prior visible preedit; host COMMIT rejection cancels its
ended session afterward. START/CANCEL/PRESENT do not format. Undo/Redo restore exact
recorded bytes and selection, bypassing format/validation while retaining resource
preparation. A formatting rewrite breaks typing coalescing and records a whole
document change; unchanged formatting preserves ordinary typing grouping.

Caret adjustment preserves the source byte-length formula, then snaps an interior
UTF8 endpoint backward to a scalar boundary. This additional CUI safety rule keeps
the portable byte-offset contract valid with Unicode literals.

## Ownership and failure rules

Pattern bytes are immutable synchronous borrowed input. Engine workspace is caller
owned and includes NUL. It must be disjoint from pattern, event payload, every model
allocation, IME descriptor/buffers, and all declared presentation spans. No hidden
per-edit allocation is added. Model scratch bounds the unformatted candidate too:
an insertion that exceeds model capacity fails even if formatting could truncate
it to fit. Formatting never silently truncates for destination capacity.

Pure helpers require disjoint destination/result descriptors and input bytes;
malformed input, alias and insufficient capacity leave output unchanged. Internal
scratch/workspace tails are unspecified; rejection preserves document, history,
selection, coalescing, session and the prior view's defined bytes plus terminator.
Active stale/wrong-model checked composition requests reject before implicit session
cancellation, preserving prior preedit on hard failures; Base still performs its
normal explicit focus/selection lifecycle cancellation. Known same-model mutators
invoked from predicates/presentation are guarded.
Caller writes and external callback side effects remain outside rollback.

For styled Input, keep workspace alive and unchanged through frame draw consumption:
its generated placeholder can be frame-borrowed. Styled placeholder capacity is
preflighted before reduction, and reused afterward. Base/engine usage only borrows
workspace during the synchronous request. No workspace pointer is retained by the
engine. Presentation resources follow their existing caller completion contract.

Nonempty patterns reject multiline models, multiple selections and inline tokens
with INVALID_ARGUMENT before writes or callbacks. Checked single-line patterns
containing CR/LF also reject before reduction, preserving CUI's existing single-line
document invariant; pure helpers may still format those literals. These are explicit
bounded CUI restrictions. Unicode `*` can split a grapheme in source formatting;
grapheme navigation otherwise keeps its established editing policy.

## Registration and proof

Integrator registers `test_input_format_engine` and `test_input_format_control`
against `cui`, and native optional `test_input_format_native` against shaped-text
libraries. The native test accepts argv font or CUI_TEST_FONT and exits 77 without
a usable font. It paints actual NotoSans bullet pixels and forces a geometry factory
failure before value/history/selection mutation. After changing host factory
availability, it sends `cui_text_metrics_changed`, honoring existing font-cache
generation invalidation. Unit scenes cover real Core/styled Input paste, paint,
semantics, IME, history, readonly and password-copy policy, plus pure source cases,
Unicode, alias, capacity, predicate/guard and prior-preedit rollback tests.

Independent public headers affected: `cui_edit.h`, `cui_base.h`,
`cui_components.h`. No additional private source/header or dependency registration.
Distribution regeneration remains integrator-owned. Numeric formatting, regex,
broader content purposes and the remaining source Input lifecycle/platform features
remain separate work.

Worker validation: strict full native build passed; CTest 142/143 passed, with
only the expected stale distribution headers failing. Three new tests passed
strict manual compilation and ASan/UBSan with leak detection. The engine test
also passed wasm32-wasi with Wasmtime49; independent public-header checks passed.
These new tests are not yet in worker CTest because registration is integrator-owned.
Logs: `/home/matteo/.cache/cui-validation/input-pattern-{build,ctest}-final.log`.
