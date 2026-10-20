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

## Private Objective-C slice: source ready, macOS validation pending

`host_macos.h` adds only opaque C factory/service types. `host_macos.m` keeps
Foundation/Speech/AVFoundation objects and ARC callback blocks private. The
native factory does not request authorization or open a microphone. Explicit
Speech preparation validates the application's nonempty
`NSSpeechRecognitionUsageDescription`; recognition rechecks authorization,
locale/on-device availability and always configures on-device-only requests,
partial results and punctuation. No microphone authorization API is called.

`host_macos_private.h` is an ObjC-only test seam. The native service fixture
injects Objective-C fake request/task/backend objects into the **actual transport**.
It is intended to observe real transport property setters, copied
AVAudioPCMBuffer samples, bounded UTF-8 conversion, oversized results,
result-plus-error precedence, cancel/end-audio counts and blocks retained beyond
host/service shutdown. It does not instantiate SFSpeechRecognizer, request OS
permission, create a real recognition task or touch a microphone. The real
Apple backend is compiled and linked in the same target but not executed.

On macOS the same standalone CMake commands add a `speech_recognizer_macos`
library and `test_macos` target with ARC/blocks, deployment target 13.0 and
strict warnings. `speech_recognizer_macos_injected` is the seventh CTest case.
CI needs the installed Xcode macOS SDK and system frameworks; no new downloaded
dependency. Run through `scripts/test_local.py` using the portable-only cache
already set by this project. The independent example currently supplies a
POSIX mutex and explicitly rejects Windows configuration.

These Objective-C sources have only been formatted/reviewed on Linux so far;
**macOS compilation and all native fixture results are pending actual macOS CI**.
Linux's six portable cases still pass after adding the conditional native build.
Do not infer native build/test success, live permission behavior, microphone
operation or recognition quality from those results. Integrator owns the CI
change and resulting evidence update.
