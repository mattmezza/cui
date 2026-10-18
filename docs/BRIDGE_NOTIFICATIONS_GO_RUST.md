# Manual queued notifications in Go and Rust

`App.PollNotificationV1()` in Go and `App::poll_notification_v1()` in Rust drain
one notification from the same native FIFO used by legacy polling and dispatch.
They invoke no handlers. `Step`/`step` already leave that queue alone; no alternate
step method is needed. Existing dispatch, frame and run behavior is unchanged.
Choose one queue owner: mixing dispatch and manual polls consumes each entry once,
in call order. These are asynchronous committed snapshots, not synchronous
controlled-component requests or binding callback subscriptions.

Go returns `(NotificationV1, bool, error)`; false with nil error means empty.
`Kind` selects activation, Boolean change, table change, surface close or Legacy;
only its applicable Boolean/Table/Surface pointer is populated. Rust returns
`Result<Option<NotificationV1>>`, with an owning enum for the same variants.
Metadata contains captured raw slot/generation identity, node kind, exact `uint64`
/ `u64` frame, `HasFrame` / `has_frame`, and the complete owning legacy snapshot.
Frames are context-local, not global timestamps or native activation identities.
Go resolves an originating Node only by its complete registered handle. Rust's
Node embeds its app identity and captured generation. Native polling skips stale
origins. A legacy selected handle stays raw: no slot-only lookup or inference that
it is still live. All protocol integers retain their unsigned widths.

Only the native initial allowlist is typed: activation, Checkbox/Switch/Toggle
committed Boolean changes, DataTable changes, and surface close. Input text,
sliders and grouped selection remain Legacy. Reserved scalar/text/selection tags
are not advertised as supported. Table columns preserve the native 1-based value
(including values greater than one); zero retains its native sentinel meaning.
The legacy snapshot includes the original textual table representation too.

```go
notification, ok, err := app.PollNotificationV1()
if err != nil { return err }
if ok && notification.Kind == cui.NotificationBooleanChangeV1 {
    checked := notification.Boolean.Value
    _ = checked
}
```

```rust
if let Some(cui_builders::NotificationV1::BooleanChange(meta, checked)) =
    app.poll_notification_v1()?
{
    let _snapshot = (meta.frame, checked);
}
```

Both wrappers verify the optional V1 symbols, advertised version bit and exact C
structure size before polling. Older shared libraries still support the old APIs;
new polling reports unavailable (Rust `Error(9)`) without draining legacy events.
Go statically linked older libraries use weak optional references on supported
GCC/Clang toolchains; Rust source builds compile the current schema. Shared-library
resolution is implemented for Unix and Windows; only Linux execution was checked
for this slice, not cross-platform runtime parity.

Text is copied into owned strings before returning. The first buffer holds 4097
bytes, followed by at most one capacity retry, capped at 1 MiB including NUL.
Oversized required capacity returns Go `ErrNotificationPayloadLimit` or Rust
`NOTIFICATION_PAYLOAD_LIMIT`; that live event remains queued. Inconsistent capacity
reports and malformed schema/tag/UTF-8 responses fail explicitly. Validation after
a successful native read cannot undo that read; no promise of retaining malformed
third-party library responses is made. No silent Legacy fallback is performed.

## Verification

`bindings/go/cui/notifications_v1_test.go` and Rust `tests/notifications_v1.rs`
exercise the real C queue: Boolean entry snapshots, FIFO/manual ownership, stale
origin removal, copied UTF-8 text and multi-column table payloads. Go also
interleaves legacy polling and verifies no automatic callbacks.
`bindings/tests/notification_v1_fixture.c` is a separate test-only shared shim:
its `CUI_NOTIFICATION_FIXTURE` modes `edges`, `capacity`, `limit`, `zero`, `repeat`,
`size`, and `unavailable` inject protocol responses. These are not evidence that
real UI producers emit oversized text or maximal frame values. Use a fresh
process per mode. It exports only V1 entry points and links the real bridge for
ordinary App APIs. On Linux Rust shared-library tests preload this shim; Go loads
it explicitly. Production libraries contain no fixture hooks.

A local external CTest harness invokes these suites through `scripts/test_local.py`
with private Xvfb. The shim compiles as C11 with `-Wall -Wextra -Werror -pedantic`.
The wrappers also compile against source-built static SDKs. Generated SDK copies
are validation inputs only; their regeneration belongs to the integrator.
