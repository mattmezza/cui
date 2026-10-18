# OTP CHANGE observation

Include `cui/cui_otp_events.h`. `cui_base_otp_events` and `cui_otp_events` add a
synchronous `{change, user}` observer to the existing Base/Styled signatures.
Their result contains the unchanged legacy `control` plus actual callback
`change_count`, zero or one. A missing callback ignores user and reduces normally.

```c
static void changed(void *user, const cui_otp_change_event *event) {
    size_t *accepted = user; /* Separate application state. */
    *accepted += event->accepted_count;
}
/* Within the normal active frame: */
/* cui_otp_events(ui, id, theme, &state, options,
 *     (cui_otp_event_handler){changed, &accepted_operations}); */
```

CHANGE means accepted operations in this submission, including empty Backspace
and changes that return to the entry text. It does not mean final text inequality.
The payload snapshots entry/final UTF-8 byte and code-point counts, target cell
length, accepted-operation and completion counts, and final completeness.
`completion_count` is the old per-operation length-match count: it can exceed one,
include deletion from overlength to target or empty Backspace at target zero,
and coexist with a finally incomplete value. No SUBMIT, verification, validity,
per-event completion callback or input-origin claim is added. Enter is unchanged.
Programmatic setters alone do not cause these notifications.

Observers run after the single real Base reduction and final semantics, before
Styled paint. Earlier accepted changes still notify if later operations report
capacity/session errors; the original error remains in `control`. Disabled,
inert and layout-only calls retain old bookkeeping/reduction without notifying.
Metadata is CHANGE at the root ID, UNKNOWN source, with only HAS_FRAME for the
active completion frame. Counts do not establish an event index or activation.

Text is final committed model bytes, borrowed only through the callback when
`has_text=true`. Masked state sets `has_text=false` and empty `text`, and never
exposes preedit. Scalar length information is retained; masking is not encryption
or length secrecy. Do not log codes/preedit or automatically persist/forward them.
Copying the event struct does not retain its text. To retain unmasked text, call
`cui_component_event_copy_utf8` inside the callback into caller-owned storage,
with an explicit maximum and capacity including NUL; check its status. Copy
failure does not reject the edit. No silent truncation or allocation is added.
The target is capped at 256 cells, but programmatic UTF-8 text can be arbitrarily
long within the supplied buffer; there is no implied 256-byte text bound.

Handler/user are borrowed only for this call; no registration/replay occurs.
Do not mutate/rebind/destroy the model, state, composition/session, context or
borrowed buffers; recursively submit; or unwind foreign exceptions during the
callback. Changing unrelated application state or setting a deferred-work flag
is allowed. Defer model changes, removal and teardown until outer submission
and renderer consumption complete: drawing may borrow model/preedit bytes.

Tests use actual queued text, keys and composition events, with paired legacy/new
semantics, focus, text-input rectangles, draw commands and consumption checks.
They cover accepted no-ops, net-zero changes, target zero, overlength deletion,
multiple completions/final incomplete text, stale duplicate commit, redaction,
explicit copies, passive/disabled behavior, bounded invalid buffers, and accepted
progress followed by errors. Native execution uses the private-display wrapper.
The old APIs, bridge LEGACY notifications and Python/Node/Go/Rust wrappers remain
unchanged; this slice adds no typed binding completion transport.
