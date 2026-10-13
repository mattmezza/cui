# Input remaining-gap audit, next27

Read-only production audit based on CUI `da398d8aab60b67e4b6ca34c7603cb395a73ef29`. Reference checkout independently reports GPUI Kit `87d10ae5e1299d1be18670a37570c5402f952e34`. This is a proposal, not an implementation or full-family parity claim. Optional builders are optional to enable; their behavior becomes required once enabled. Gallery examples do not substitute for reusable control contracts.

## Source evidence and current scope

### acceptance — implemented bounded subset

da398d8 checks candidate then old, empty always valid, history and multiline bypass; preedit checked. Explicit invalid/required are separate. CUI callback protects same-model mutation unlike source mutable App callback.

Evidence: `crates/base/src/input/base/state.rs:3534–3553,4109–4124,4265–4280`.

### normalization — missing default source behavior

Single-line source removes both CR and LF before candidate validation and default-value setup. CUI returns LINE_BREAK for CR/LF. U+2028 is not removed by that source helper. Number-mask normalization is conditional, not generic.

Evidence: `crates/base/src/input/base/state.rs:3520–3531,1241–1243,3996–4002,4243–4249`.

### format masks — missing opt-in feature

9 ASCII digit, A ASCII letter, # ASCII alphanumeric, * any scalar, other scalars separators. Prefixes are valid; formatting adds/skips literals. Number variant supports separators/fraction formatting. Mask rewrites document, adjusts caret and records whole-document atomic history when changed. Not password hiding.

Evidence: `crates/base/src/input/base/mask_pattern.rs:100–165,172–245,283–410; base/src/input/base/state.rs:4125–4160`.

### regex — missing opt-in feature

Source regex::Regex is_match is search matching unless expression anchors itself. Validator runs first, then mask, then regex; empty bypasses all. Existing CUI callback can host an application predicate but is not a portable regex implementation or regex source compatibility.

Evidence: `crates/base/src/input/base/state.rs:3534–3553,10539–10552`.

### password — missing opt-in feature with mandatory security behavior when enabled

Source displays MASK_CHAR once per Unicode scalar, maps byte ranges to display offsets, double-click selects all, disables Copy and Cut, withholds accessibility value. CUI styled Input has no masked option; current plaintext drawing/clipboard/semantics must not be relabeled password support.

Evidence: `crates/base/src/input/base/element.rs:343–362; base/src/input/base/selection.rs:24–35; base/src/input/base/mod.rs:105–111; component/src/input/input.rs:89–96`.

### content type — missing opt-in semantic and platform feature

Source has 45 enum variants including name/address/credit card/credentials/OTP/date/cellular metadata. Styled role maps telephone/email/URL/password/date types; password/new-password suppress value exposure independently of masked drawing. Native NSTexContent seam is macOS-only; nonmac sync is no-op. SDL/browser autofill and keyboard hints require separate host capabilities, not portable metadata alone.

Evidence: `crates/component/src/input/content_type.rs:7–98,157–174; component/src/input/input.rs:27–96; base/src/input/base/native.rs:1–108`.

### shortcuts — portable common subset, platform gaps

Common clipboard/select-all/grapheme/word/history/home/end exists; single-line vertical no-op and popup delegation tested. CUI combines Ctrl/Alt word operations regardless of OS; source macOS Ctrl-Backspace is scalar/grapheme Backspace, Cmd-Backspace/Delete delete to line edge, Cmd-Left/Right home/end, Ctrl-A/E home/end. Source character palette, Escape/search/replace/context actions are not implemented in Input. Editor-only actions are not automatically single-line requirements.

Evidence: `crates/base/src/input/base/state.rs:135–306; CUI src/base/cui_edit.c:5152–5223; src/base/cui_base.c:312–329`.

### pointer selection — partial composed path

CUI click/Shift click/drag hits byte geometry and snaps edit boundaries. Source double-click word and triple-click line are separate selection modes; no equivalent click-count reducer exists in styled Input. Masked double-click has special all-selection privacy policy.

Evidence: `crates/base/src/input/base/selection.rs:24–63; CUI src/base/cui_base.c:333–353`.

### lifecycle — state tracking subset, source event API missing

Source Focus/Blur/Change/PressEnter events, cursor blink start/stop and context-menu blur exception are explicit. CUI result changed/submitted plus retained focus grouping cover part; no corresponding event callback stream or cursor blink timer. Existing immediate result callers can observe flags but must not claim source event ordering.

Evidence: `crates/base/src/input/mod.rs:125–131; base/src/input/base/state.rs:3389–3418,4208–4209`.

### platform paste — bounded synchronous seam, source async gap

Source async permission-gated paste rechecks editability, focused handle, document revision and selections before applying late result. CUI platform clipboard API is synchronous borrowed text; arbitrary delayed completion needs an explicit generation/target transaction. Current web host composition bridge does not prove async clipboard equivalence.

Evidence: `crates/base/src/input/base/state.rs:2681–2710`.

Primary pinned files: [state.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/state.rs), [mask_pattern.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/mask_pattern.rs), [selection.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/selection.rs), [input.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/input/input.rs), [content_type.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/input/content_type.rs), [native.rs](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/native.rs).

## Smallest next implementation slice: bounded CR/LF normalization

Implement only source single-line CR/LF stripping before acceptance, including typed text, paste and preedit/commit. Do not add number-mask Unicode transformations without Number mask configuration. Preserve raw engine callers and multiline behavior; default programmatic CUI create remains raw unless the caller explicitly normalizes its seed. That remaining default-value difference must stay documented.

Proposed exact portable API additions, subject to integrator approval:

```c
typedef enum {
    CUI_EDIT_NORMALIZE_NONE = 0,
    CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF
} cui_edit_normalization;
/* Append to cui_edit_request and Input/Base Input options: */
cui_edit_normalization normalization;
/* Bounded explicit seed normalization; no hidden allocation: */
cui_edit_result cui_edit_normalize_text(cui_str source,
    cui_edit_normalization policy, char *destination, size_t capacity,
    cui_str *normalized);
```

Zero keeps the existing raw/LINE_BREAK policy. The source-compatible Input setup explicitly selects CRLF stripping. The helper rejects malformed UTF-8, enum errors and aliases; on capacity/argument failure destination and output remain unchanged. Output borrows destination; include NUL capacity and checked size arithmetic. Source text remains immutable. Reject destination overlap with source or output descriptor before writes; no in-place contract. No Unicode NFC/NFKC or U+2028/U+2029 transformation.

Checked engine dispatch should build the full resulting value directly into its existing workspace while skipping CR/LF, rather than materializing a payload in workspace and then overwriting that payload during candidate construction. Count/check capacity before writes. A checked request with normalization enabled must take its preparation path even if the predicate is NULL; the current NULL-predicate legacy fast path cannot silently bypass normalization. Multiline checked dispatch ignores the single-line policy. A successful edit uses normalized byte counts for selection/history. Normalize provisional selected anchor/cursor by the count of surviving bytes before each original valid UTF-8 endpoint. This mapping is monotonic and must be applied before composition range validation against normalized text. Retain existing preedit bounds and fixed/growing ownership guarantees; excessive original host payload must still follow the documented preedit-capacity policy. Do not temporarily mutate borrowed Core event text. History replay bypasses normalization and validation; rejected updates preserve the prior visible value, rejected host commits cancel only afterward.

Tests must use actual styled/Core scenes for typed CRLF, paste `a\r\nb`, selection replacement, IME UPDATE/COMMIT with selected ranges before/inside/after removed characters, empty-normalized update/cancel, rejected normalized candidate, one Undo/Redo and healing. Include document/view capacity edges, non-ASCII emoji surrounding breaks, U+2028 retention, malformed/alias failures, readonly/disabled, zero-policy legacy LINE_BREAK and multiline retention. Seed helper tests must cover full output/terminator rollback and caller storage lifetimes. Native/browser dispatch tests should follow after host ownership is assigned; no physical OS keyboard/candidate claim follows from headless tests.

Suggested ownership: edit.h/edit.c normalization preparation; Input-only Base/Components options/forwarding; new composed unit test and guide. No Core or host changes are necessary for this first slice. Later password work needs Core semantics exposure controls, renderer masking with byte mapping, clipboard suppression and browser/native accessibility ownership together; a drawing-only option would be incomplete. Later mask formatting needs a separate bounded formatter plus whole-document atomic history/caret mapping. A regex provider should be explicitly opt-in and dependency-pinned, or expose a documented external predicate adapter without claiming regex parity.

## Validation receipt

Strict C11 build passes with `-Wall -Wextra -Wpedantic -Werror`. CTest passes 111/112; only `distribution` fails because generated files are stale. Counts and commands are recorded in the adjacent JSON receipt. New acceptance test is intentionally not registered in this base; its original scoped commit supplied independent strict/sanitizer/WASI evidence. Generated distribution belongs to the integrator and is expected stale after source-only acceptance changes. No production files were modified.
