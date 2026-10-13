# Bounded Input normalization

Source pin: GPUI Kit `87d10ae5e1299d1be18670a37570c5402f952e34`,
[state.rs normalize_input](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/state.rs#L3520).
Its single-line path removes CR and LF before replacement and validation. Number
mask Unicode normalization is separate. This slice implements only explicit
CR/LF removal; it does not implement format masks, regex or password presentation.

Set `.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF` on styled/Base Input
options or a `cui_edit_request`. Zero preserves existing `CUI_EDIT_LINE_BREAK`
errors. Multiline checked edits retain CR/LF and bypass this single-line policy.
Raw edit APIs and model creation retain their existing behavior. Applications
must explicitly opt into source-compatible normalization; no automatic seed
conversion is claimed.

For a bounded initial value, call:

```c
char seed[128];
cui_str value;
cui_edit_result error = cui_edit_normalize_text(cui_string("a\r\nb"),
    CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF, seed, sizeof seed, &value);
/* On success value borrows seed and contains "ab". Create the model from seed. */
```

The helper validates the complete original UTF-8 value and enum, checks all known
source/destination/output-descriptor overlaps, then verifies capacity including
NUL before writing. Failure leaves destination and output unchanged. No in-place
or overlapping operation is supported. It owns no storage; source remains
immutable and the returned value lives as long as destination. Unicode scalar
bytes are unchanged: removal matches ASCII CR/LF only, not U+2028, U+2029 or other
whitespace. Empty normalized output is valid. This helper is independent of a
model; writing any caller-owned model buffer during a predicate remains forbidden
by the existing borrowing contract.

Checked edits use existing engine/session workspaces without additional staging
allocation. Ordinary document capacity applies to the normalized resulting value,
so a long CR/LF-rich original payload can fit a smaller document. IME original
payload length must first fit caller-owned preedit capacity; removal does not
bypass that input bound. Fixed/growing rendered-view capacity applies to the
normalized full view. Existing lazy growth occurs only when that view needs it.

For IME, anchor/cursor are validated against the original UTF-8 payload first,
including scalar boundaries. Each endpoint maps to the count of surviving bytes
before it. The resulting session stores normalized preedit and normalized byte
ranges, not original-host provenance. Neither original Core event strings nor
host offsets are rewritten. UPDATE does not mutate the committed document;
COMMIT creates one Undo step. Empty updates preserve the existing CUI active-empty
session contract until explicit CANCEL/COMMIT; this does not claim source empty
marked-text lifecycle equivalence.

Whole-value acceptance sees normalized candidates and the previous visible value.
Rejected updates retain prior preedit/view bytes and ownership. Engine rejected
COMMIT retains its prepared session; the built-in host reducer subsequently
cancels because the host ended composition. UTF-8, range, capacity and alias
errors precede mutation where possible. Same-model predicate mutation remains
guarded, and no callback is replayed while committing. Undo/Redo restore recorded
values without normalization or acceptance. Normalized typing coalesces using its
existing intent; paste and IME retain atomic history behavior.

The composed test `test_input_normalization.c` covers typed/pasted selection
replacement, grouped Undo/Redo, normalized rejection, readonly/disabled and zero
policy, Unicode/emoji endpoint mapping, fixed/growing IME views, empty update and
cancel, helper aliases/rollback, model capacity and guarded destruction. It runs
through CTest and the portable WASI runner. The website's validated Input example
also exercises ordinary typing and native browser IME rejection/recovery on
desktop and a 390px viewport. Browser clipboard paste, physical keyboard
candidate windows, autofill and complete Input parity remain unverified.
