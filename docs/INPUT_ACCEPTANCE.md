# Whole-value Input acceptance

The contract follows pinned GPUI Kit commit
`87d10ae5e1299d1be18670a37570c5402f952e34`,
[replacement and preedit validation](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/state.rs)
(`is_valid_input`, `replace_text_in_range`, `replace_and_mark_text_in_range`).
This implements a bounded predicate for the single-line composed Input, not full
Input-family parity or source regex/mask/content-type behavior.

Set `cui_input_options.validate` and `validate_user` (or the corresponding Base
options). The callback receives the complete proposed UTF-8 value, never merely
the inserted payload. Empty candidates are implicitly valid. Otherwise call the
candidate predicate first; only if it returns false is the previous value tested.
Reject only when that previous value is valid. This permits repairing invalid
programmatic defaults without trapping the user. Programmatic legacy edit APIs
remain unvalidated; history Undo/Redo bypass acceptance, as does a multiline model.
Multi-selection attachment is currently a multiline extension, so it retains its
existing behavior without this source single-line predicate; no validated
multi-cursor extension is claimed.

```c
static bool digits(void *unused, cui_str value) {
    (void)unused;
    for (size_t i = 0; i < value.length; ++i)
        if (value.data[i] < '0' || value.data[i] > '9') return false;
    return true;
}
/* state and edit are retained caller-owned objects. */
cui_input_result result = cui_input_ex(ui, id, theme, cui_string("Digits"), edit,
    &state, (cui_input_options){.validate = digits});
if (result.rejected) { /* An edit was declined, without changing the value. */ }
```

A declined edit sets `result.rejected` with `error == CUI_EDIT_OK`; it does not
implicitly mark the control invalid. `invalid` remains an explicit semantic/paint
flag and `required` remains semantic metadata. UTF-8, boundary, capacity and
forbidden single-line line-break errors remain distinct and publish the existing
same-frame error appearance. The source strips CR/LF from single-line insertion;
CUI retains its explicit `CUI_EDIT_LINE_BREAK` engine policy in this batch.

## Engine and host dispatch

`cui_edit_apply_checked` takes a by-value request and acceptance policy, returning
`{error, rejected}`. EVENT delegates existing keyboard/text behavior, INSERT is
atomic clipboard insertion, DELETE_SELECTION removes a selected cut range, and
COMPOSITION applies the reusable IME session. Invalid request kinds/null models
fail without callback invocation. Raw legacy APIs and null-policy checked requests
keep their existing behavior; no permanent callback/userdata is retained in the
model. Invalid UTF-8/boundaries/capacity fail before a predicate is invoked.

The actual Input reducer routes typing, paste, selected/grapheme/word erasure and
composition through this seam. Copy is unchanged. Cut copies first, then attempts
validated deletion, matching source `cut`: a rejected cut can still copy its
selection. Clipboard failure leaves the document unchanged and does not invoke
the predicate. Readonly/disabled controls retain existing mutation guards.

IME UPDATE validates a complete provisional view without changing committed text
or history. The previous predicate value is the prior composed view when active,
not merely the committed document. Rejected updates preserve that view. Engine
COMMIT rejection also preserves session state; its standalone caller may retain
or cancel it. The built-in Input reducer then explicitly cancels a rejected host
COMMIT as a subsequent lifecycle transition: native/browser composition has ended,
corresponding to source `unmark_text`. Committed text, selection and history remain
unchanged, and following typing/shortcuts work. A successful commit is one Undo step.
START/CANCEL and history replay do not invoke the predicate. Existing focus-loss,
readonly/disabled and source-revision cancellation policies remain intact.

## Ownership, bounds and rollback

There is no additional acceptance allocation or caller preview buffer. Ordinary
edits/commit reuse the model's existing creation-time scratch, bounded by document
capacity. Provisional views reuse existing fixed caller view storage or the opted-in
model-owned `grow_view` cache, bounded by document capacity + preedit capacity - 1.
The existing grow cache can allocate on necessary growth, not on every edit.
Fixed-view capacity remains an explicit error; oversized preedit is not truncated.
The engine validates known workspace/session/model aliases before writes. Payload
slices of the stable document or preedit buffer are supported; payloads overlapping
the working candidate workspace are invalid.

Each predicate borrows an immutable value only for that synchronous call. On a
failed candidate the shared view may be rebuilt from unchanged document/preedit
storage for the old-value predicate, then restored or rebuilt for acceptance.
Predicates are never replayed as part of commit. Rejection preserves logical model
metadata, coalescing, selection, history/redo, annotations and session ownership,
and the previous view's defined bytes through its NUL terminator. Internal scratch
and unused capacity tails are unspecified; this is not an entire-allocation byte
identity promise. No idle-frame document copy is added.

Same-model mutation during a predicate is guarded: mutating APIs and scratch/cache
operations return INVALID_ARGUMENT; void destroy/detach/coalescing setters do
nothing and record the violation. The outer checked request rejects with
INVALID_ARGUMENT, restores its logical prepared state and leaves the model alive.
Destroy it normally after the checked call returns. Pure getters for text/state,
content, selections and columns remain available. Nested same-model checked calls
are rejected. Other models, UI/host callbacks and external side effects are not
blocked or rolled back. Direct writes to caller buffers or borrowed strings,
freeing caller storage, retaining candidate pointers and cross-thread model access
violate the borrowing contract and cannot be protected by a C callback guard.

## Validation and integration

`tests/unit/test_input_acceptance.c` exercises actual composed Input/Core dispatch
for typing, paste/copy-first cut, grapheme/word/selected erasure, healing, history
replay/redo retention, readonly/disabled/error separation and IME host lifecycle.
It also checks same-model mutation/destruction guards, malformed/capacity/alias
requests, fixed/growing provisional views larger than committed capacity, prior
view restoration, rejected growth, a 200 KB whole-value candidate and multiline
bypass. Register it linked only to `cui`, including
WASI. Root owns CMake, distribution and registry changes; this source batch does
not claim full native/browser accessibility or all Input reference features.
