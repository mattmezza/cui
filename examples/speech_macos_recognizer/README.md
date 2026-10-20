# Optional macOS Speech recognizer host

Portable application-host slices are implemented with an injected service.
Native Objective-C service execution, macOS compilation, permission dialogs and
live recognition remain deferred. This is not a native-recognition support or
full Speech parity claim. See [the design and native validation plan](../../docs/design/SPEECH_MACOS_RECOGNIZER.md).

`host.h` exposes only C11/CUI types. `host.c` contains the bounded callback pool,
permission/session state and transcript normalization. `host_lock.c` privately
supplies POSIX mutexes for Linux/macOS. No Core/public framework API changes,
recognizer engine downloads, networking, microphone capture or OS permission
requests occur in these portable fixtures.

## Contract

Create the host with a service vtable, private mutex operations and an optional
nonblocking wake. Constructor/query/refresh/available never request permission.
`speech_recognizer_host_prepare` is an **explicit user action** that may request
Speech consent in a native service; it checks the usage description and allows
only one pending request. Microphone permission remains a separate
application/SDL-input concern. Native service operations must enforce main/UI
thread ownership; generic C11 code assumes one owner for host operations.

The service must report support, usage-description presence, locale validity,
on-device capability/availability and authorization. The host always passes
`on_device=true` to start. No network fallback is accepted. `available` uses the
last refreshed service snapshot plus pending-permission and pool capacity.
Refresh on the UI timer; start refreshes again. Fixed format is 16000 Hz mono
PCM16: service-format mismatch makes available false/start UNSUPPORTED. Mutating
the returned provider's format is unsupported; start also checks the CUI state's
actual format and returns INVALID if it was changed, even if the independent
host's cached available predicate remained true. No conversion is hidden here.

Each service operation gets a borrowed endpoint. Retain it before an asynchronous
callback stores it; release the lease when its callback/block is destroyed, even
if it never fires. `close` releases each opaque service token once. An already
retained callback lease may survive close and host destruction. It must never
use an endpoint after releasing its own final lease. Endpoints contain no host,
CUI sink or independent wake-userdata pointer capture.

The fixed allocation has four endpoint slots, each with eight event slots and
at most 4095 UTF-8 payload bytes. Oversized/malformed text is converted to a
queued terminal error; callback return means the event/error was queued, not
that recognition succeeded. Queue overflow and wake failure use a separate
terminal latch. Active operations, pending permission and retired callbacks
share the four slots. Retired slots are never reused until all leases release;
if a service never releases callbacks, capacity stays exhausted rather than
allocating replacements or recycling dangling pointers. Closed callbacks are
rejected without touching a sink or wake userdata. Detach waits out an existing
short wake under the mailbox mutex. Wake must not block, reenter the host, or
wait for owner/UI dispatch.

Poll the recognizer and optional input host on the owner thread, then call
`cui_speech_pump` with monotonic time, including during STOPPING. UI polling has
an explicit event budget. It reports READY after task creation, not from an OS
callback. Appends copy borrowed PCM in at most 256-sample chunks before the
service returns. The service must copy/consume its borrowed span synchronously.
Finish sends end-audio once; CUI timeout/cancel/final invokes provider close.
Dispose/cancel CUI before destroying its provider host. Host destruction rejects
an active CUI session and detaches pending authorization safely.

Normalization handles cumulative hypothesis replacement, metadata-ended
utterance resets using Unicode codepoint comparison, English punctuation and
CJK spacing. Final text commits once; an empty final clears an old hypothesis.
Text plus error preserves text as hypothesis and fails, even when marked final.
The private Apple no-speech error exception is deliberately unsupported until
separate native evidence exists. OS-specific transcription variations remain
unverified; these fixtures do not establish full upstream utterance parity.

Permission completion failures are observable via
`speech_recognizer_host_last_error`; synchronous prepare failures are returned
by prepare. The caller must not treat denied/overflow/failure as successful
consent. Permission authorization can change later; refresh plus polling fails
an active session on revoked authorization/service availability without
preventing cancellation or finish cleanup.

## Independent offline validation

Use an existing compiler/CMake/POSIX thread implementation; no install or fetch:

```sh
cmake -S examples/speech_macos_recognizer -B "$HOME/.cache/cui-validation/speech-recognizer" -G Ninja
cmake --build "$HOME/.cache/cui-validation/speech-recognizer"
TMPDIR="$HOME/.cache/cui-validation" python scripts/test_local.py \
  --build-dir "$HOME/.cache/cui-validation/speech-recognizer"
```

The standalone build sets `CUI_BUILD_NATIVE=OFF` for the wrapper's portable-only
policy and compiles only host/base/fixture sources. Linux still uses private
Xvfb; no desktop fallback. Six CTest cases cover permission/status gating,
transcript revisions/resets/UTF-8, lifecycle/format/PCM copying, queue/text/pool
capacity and wake failures, two concurrent result producers with owner-thread
CUI delivery, and callbacks after actual host/wake-userdata free. Pending
permission destruction and permanently retained leases are injected explicitly.

Verified on Linux/GCC 16.2 in strict C11 with
`-Wall -Wextra -Wpedantic -Werror`: 6/6 wrapper tests pass. The same six tests
pass with AddressSanitizer and UndefinedBehaviorSanitizer, including late leases
and final pool reclamation. No full repository/package/skill tests were run for
this isolated optional example; integration checks belong to the integrator.

## Native service work remaining

Private Objective-C ARC/blocks transport will link Foundation, Speech and
AVFoundation with deployment target macOS 13.0+. Native compile/link and injected
ObjC service fixture need actual macOS CI; Linux fixtures do not prove them.
Real microphone consent/capture, Speech authorization, locale/model availability,
on-device recognition and real stop/no-speech behavior remain explicit manual
validation. None is executed automatically or inferred from portable tests.
