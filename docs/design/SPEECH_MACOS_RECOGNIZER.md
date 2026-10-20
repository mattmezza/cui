# Optional macOS Speech recognizer: design checkpoint

Status: bounded portable host and private Objective-C transport implemented.
Portable integration fixtures pass 6/6; actual Apple Debug and Release CI passed
7/7 each ([run](https://github.com/mattmezza/cui/actions/runs/39396253418), source
`4ece99ec`). The proposed contract below records design decisions; implemented
names and current instructions are in the [example guide](../../examples/speech_macos_recognizer/README.md).
No Core/public framework API or input-dispatch changes. Real permissions,
microphone capture and on-device recognition remain unverified.

## Decision and authority

Implement an independent application-host `cui_speech_recognizer` adapter:
portable C11 session/transcript/mailbox logic plus a private Objective-C ARC
Speech/AVFoundation transport. Reuse the optional SDL input provider for PCM;
the recognizer never opens a microphone. Start with an explicit macOS 13+
minimum for this optional example, not a new framework minimum or a claim about
older macOS support. No network client, model/runtime dependency, threadpool,
or Objective-C type enters Core or a public framework header.

The comparison remains GPUI Kit 0.7.1 at
[`87d10ae5e1299d1be18670a37570c5402f952e34`](https://github.com/longbridge/gpui-kit/tree/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/speech/system),
verified from the local checkout. Its `macos.rs` uses an on-device request,
permission completion messages, partial/final transcription, utterance-reset
handling, and a stop-with-no-speech exception. Its channel and preauthorization
PCM buffering are unbounded; those choices must not become CUI host contracts.
Existing [native host analysis](SPEECH_NATIVE_HOST.md),
`include/cui/cui_speech.h`, and `src/base/cui_speech.c` are the CUI authorities.

Apple's current primary documentation was checked on 2026-10-20:

- [requestAuthorization](https://developer.apple.com/documentation/speech/sfspeechrecognizer/requestauthorization(_:)) is asynchronous; its completion need not run on the main queue. A valid `NSSpeechRecognitionUsageDescription` is required before asking.
- [supportsOnDeviceRecognition](https://developer.apple.com/documentation/speech/sfspeechrecognizer/supportsondevicerecognition) must be true for a request to honor [requiresOnDeviceRecognition](https://developer.apple.com/documentation/speech/sfspeechrecognitionrequest/requiresondevicerecognition). Both checks are mandatory; no silent network fallback.
- [authorizationStatus](https://developer.apple.com/documentation/speech/sfspeechrecognizer/authorizationstatus()) reports permission that can change later.
- [recognitionTask](https://developer.apple.com/documentation/speech/sfspeechrecognizer/recognitiontask(with:resulthandler:)) delivers asynchronous partial/final/error callbacks. [cancel](https://developer.apple.com/documentation/speech/sfspeechrecognitiontask/cancel()) does not document a synchronous callback-quiescence barrier. This design does not invent one.

## Permission preparation and availability

Permission preparation is separate from recognizer session start. Unchanged CUI
calls recognizer `start` and then immediately starts the input provider, without
waiting for READY. Therefore an asynchronous permission request inside `start`
would capture audio while consent is pending or require a second PCM queue.
Choose explicit preparation instead; no PCM is buffered awaiting authorization.
This deliberately differs from frozen upstream's first-start permission flow.

A proposed application-host `prepare_authorization` operation may request
**Speech recognition permission only**, from an explicit user action. Constructor,
provider factory, `available`, status refresh, poll, tests, and CI never prompt.
Before requesting, validate a nonempty usage description. Missing/invalid key
returns UNSUPPORTED locally and must not call Apple's request method. At most
one permission request per host can be pending; another returns INVALID at the
example host API level. A request is not made again merely because the UI polls.

Microphone consent is independent. Speech authorization never proves microphone
permission. The application/SDL capture host separately owns
`NSMicrophoneUsageDescription`, microphone consent and device access. The
recognizer requests none of those capabilities and cannot mark them granted.
The application must arrange any microphone preparation in its own explicit
user flow before recording, or knowingly handle an input-provider failure.

Owner-thread status refresh produces a cached, reasoned snapshot. Proposed
reasons are unsupported OS/service, invalid locale, missing usage description,
permission undetermined/pending/denied/restricted, on-device unavailable,
recognizer temporarily unavailable, callback capacity exhausted, and ready.
`available` returns the cheap cached Speech-ready predicate and never prompts.
It does not claim device availability, microphone consent or future success.
`start` rechecks current Speech status and on-device support to close the race
between render-time availability and activation.

| Speech condition | `available` | Session `start` | Explicit preparation |
| --- | --- | --- | --- |
| Missing description/invalid locale/unsupported OS | false | UNSUPPORTED | local failure, no Apple request |
| Undetermined or permission completion pending | false | PERMISSION | one pending request permitted |
| Denied/restricted | false | PERMISSION | returns current status; no automatic retry loop |
| Authorized, on-device unavailable | false | UNSUPPORTED | no network/model download fallback |
| Authorized, recognizer temporarily unavailable | false | UNSUPPORTED | refresh later, no automatic session restart |
| Authorized and on-device ready, lease pool has space | true | attempt task creation | no prompt needed |
| Callback lease pool exhausted | false | CAPACITY | no new permission/task request |

An availability/authorization change during an active task updates the cached
status via explicit refresh and a bounded event where relevant. It must not
prevent `finish`, `close`, cancel or timeout cleanup. On revoked permission or
lost on-device service, report a terminal permission/recognizer error. Do not
silently switch service, locale or task underneath an existing session.

## Service seam and bounded event transport

All names below describe proposed example APIs, not existing CUI symbols.
An opaque host provides `provider`, `prepare_authorization`, `refresh`, `poll`,
status query and destroy. Its public header contains only portable C11/CUI types.
An injected service vtable exposes snapshot/query, request authorization,
start-task, append-PCM, end-audio and cancel/release. Every task operation runs
on the owner/UI thread; for this native example that owner is the macOS main
thread. Reject native owner operations from another thread rather than silently
dispatching synchronously. Completion handlers can run concurrently on arbitrary
queues. Service tokens are opaque. No OS callback receives a CUI sink or host
pointer.

Use an independently retained endpoint for each permission action or recognition
task. A fixed pool (proposed default four endpoints) is allocated once; each
endpoint has eight fixed event slots with at most 4096 UTF-8 bytes per result,
plus one terminal/error latch outside queue storage. Configuration must bound
pool size, slots and text bytes before allocation, with checked arithmetic.
All active **and retired** endpoints, including pending permission actions,
consume the same pool. One recognition session is active per host.

Callbacks may be concurrent producers; use a short private mutex through an
injected lock seam, not the SDL input's SPSC assumption. The C11 state/mailbox
logic sees only lock/unlock callbacks and opaque storage. Linux fixtures use a
private pthread mutex; Objective-C supplies its private mutex implementation.
No platform type leaks into the example's public header. UI poll copies a
bounded event to owner scratch, unlocks, then reports to CUI. Never invoke CUI,
service operations, user event handlers, or lifecycle calls under that mutex.

Callback processing validates/copies a bounded UTF-8 snapshot, metadata flags
and classified error into its endpoint. Do not allocate an unbounded temporary
`UTF8String` copy or duplicate an arbitrary NSError description first. A text
that cannot fit, event queue exhaustion, or conversion failure latches a visible
terminal CAPACITY/RECOGNIZER_ERROR; later payloads are discarded. No silent
hypothesis dropping is part of the initial contract. Framework-owned objects
and internal OS buffering are outside this host-owned memory bound.

An optional nonblocking wake may post a UI notification; failure latches a
terminal recognizer error. Use no wake coalescing initially. A short UI timer
continues polling for wake failures, stale availability and CUI stop deadlines.
Wakes may not call poll/lifecycle synchronously or wait for owner dispatch.
The endpoint mutex gates use of wake userdata: close waits out an in-progress
short wake and detaches it before returning. Late callbacks then have no access
to application wake userdata. This narrow wake gate is an explicit exception
to the rule against arbitrary user callbacks under the mailbox lock; wake is
restricted to a nonblocking notification operation, not a general user hook.

## Lifetime and cancellation without a fictitious Apple barrier

A task callback block strongly retains only an endpoint lease object. It never
captures the host, CUI state, raw sink, wake userdata separately, task wrapper,
or a closure that captures them. This avoids callback/task retain cycles and
allows callbacks after cancellation without retaining disposed application data.
The endpoint's parent pool has independent reference-counted lifetime.

1. `start` reserves a free endpoint, captures the CUI sink in owner-only host
   session state, creates an on-device request/task and returns one opaque
   successful session token. On failure it cancels/releases every acquired
   service object and detaches the endpoint; any already-retained callback
   lease becomes a retired pool occupant.
2. Endpoint publication precedes calls that can complete synchronously. Service
   callbacks enqueue only; READY is reported by owner poll after successful
   task creation. It means the task accepts audio, not that text was recognized.
3. `close` marks the endpoint closed under its mutex, clears pending events and
   wake/userdata, and drops the host's raw sink/session association. It then
   cancels/releases task/request ownership on the owner thread exactly once.
   It never waits for Apple completion, a dispatch queue, or a UI-dispatched
   callback. Duplicate close cannot cancel/release a token twice.
4. A callback already copying/posting completes before endpoint detachment
   finishes. A later callback can touch only its retained closed endpoint; it
   discards the event and does not wake/report. Physical OS callback quiescence
   is not required for CUI state or host destruction to be safe.
5. An endpoint can be reused only after every callback lease is released.
   The service owns/releases its callback lease even if the callback never
   fires, through block/lease-object destruction. Slot reuse is withheld until every lease releases, so a valid old callback
   cannot observe a reused slot. No endpoint generation is needed under that
   lease contract; using a pointer after releasing its last lease is invalid.
   CUI still rejects queued reports from previous CUI session generations.
6. If Apple never releases a block, its endpoint remains retired. After the
   fixed pool is exhausted, new starts and permission actions fail CAPACITY;
   there is no timeout-based unsafe reuse or unlimited replacement allocation.
   Destroy can detach/free owner-only host data while outstanding leases retain
   the fixed pool. The pool is reclaimed only after the last lease releases.
   This bounds host-owned retention per instance; it does not promise that OS
   task resources disappear or globally bound repeatedly created host objects.

The CUI raw sink is never stored in an endpoint or captured by a block.
Generation rejection is useful only while CUI state lives; it is not the
lifetime mechanism. `cui_speech_dispose` closes an active provider; application
shutdown must dispose/cancel CUI before destroying provider host ownership.
Pending authorization has no CUI session and is detached by host destruction.
Already visible OS consent UI cannot be withdrawn by claiming task cancellation.

## PCM, result and stop semantics

Initial format is fixed 16000 Hz mono signed PCM16, matching existing CUI and
SDL speech defaults. `push_audio` consumes its borrowed span synchronously;
private ObjC creates bounded AVAudioPCMBuffer chunks, copies samples, appends
them, then releases local ownership. No preauthorization PCM queue exists.
No AVAudioEngine or second microphone capture is added. A framework append API
without backpressure cannot establish a bound on Apple's internal audio queue;
limit application session duration/input delivery and document that limit
rather than claiming the endpoint bound covers OS PCM buffering.

`finish` calls `endAudio` once, stops accepting new PCM, and leaves result
polling active. CUI already closes capture first and enforces its monotonic
stop deadline through `pump`. `close` is cancellation/release, not another
successful final event. Final callbacks arriving without an explicit user stop
may finish normally; CUI then closes capture and recognizer through its existing
state machine. Availability becoming false must not disable cleanup.

Normalize results in testable owner-thread C11 logic. Partial text replaces the
current hypothesis. A final commits exactly once, then reports finish. Frozen
upstream's utterance reset heuristic needs explicit fixtures: metadata-ended
utterance followed by revised cumulative text versus a new utterance, Unicode
codepoint prefix comparison, punctuation and CJK separators. Keep bounded
previous utterance/hypothesis storage. If a first implementation supports only
cumulative results, document that limitation and keep parity partial until the
reset fixtures pass. Do not copy Rust runtime/channel behavior into CUI.

For a callback containing both result and error, copy both into one bounded
event and give the error precedence: report any supplied text as a hypothesis,
then report a terminal recognizer error; do not commit or report successful
finish, even if the result says final. A result without error completes once
when final. This conservative rule differs from any upstream path that drops an
error after processing final; cover that difference explicitly. No error is
silently ignored based solely on callback timing.
The frozen private no-speech domain/code exception is unverified Apple behavior;
treat errors as recognizer errors initially. Adding a finish-with-no-speech
exception requires a separately pinned fixture and documented limitation.
CUI report queue failure must lead to its existing capacity terminal handling,
not a false successful finish after a dropped phrase.

## Coverage and implementation slices

Integrator approved the checkpoint before production work. Portable slices 1/2
now have strict C11 and ASan/UBSan proof through the private test wrapper; the
remaining native slices still require actual macOS validation. Constants are
fixed at four endpoints/eight events/4096-byte storage rather than configurable
budgets in this first implementation. No additional allocation occurs on result
publication. The implementation covers selected Unicode/utterance fixtures; it
does not claim all OS transcript behavior or the private no-speech exception.

| Slice | Deliverable | Meaningful evidence and stop condition |
| --- | --- | --- |
| 1 | portable host/status machine, service vtable, retained bounded endpoint pool | Linux injected permission/status table, missing-key request counter stays zero, concurrent producer order, queue/text overflow, wake failure, lease exhaustion, late callback after close/free, exactly-once cancel/release; no OS/device calls |
| 2 | bounded transcription normalization and CUI provider integration | ready/partial/final order, synchronous service completion, PCM span copy, utterance revisions/resets/CJK, combined result/error, cancel/restart generations, stop timeout without completion, capacity failure, availability revocation during finish |
| 3 | private ObjC transport compiled with ARC/blocks, Speech/AVFoundation/Foundation | macOS compile/link with strict warnings and deployment target 13.0; headless injected ObjC service fixture exercises class/protocol seam, callback-block retain/release, bounded UTF-8 conversion, PCM buffer samples, request policy setters and shutdown; no permission request/real recognition task/microphone |
| 4 | opt-in application wiring/manual validation instructions | explicit user consent, real on-device locale/service, capture permission, cancellation, no-speech stop, device loss; deferred until actually exercised on macOS hardware |

Standalone external CMake must compile only necessary host/base/fixture targets.
Linux fixtures run through `scripts/test_local.py` private Xvfb. macOS CI uses
the existing headless test policy or an explicitly portable-only cache setting;
no window, permission dialog, audio device, recognition request, network/model
asset installation or real user audio is allowed in automatic fixtures. Native
production symbols can be linked for compile proof while injected service
factories replace all OS permission/task execution. Real policy setters should
be observed through the fake native-service seam, not inferred from a fake C
provider that never compiles the ObjC transport.

No broad rebuild, packaging or skill-generation work belongs to this isolated
proposal. Integrator owns metadata/skill/CI changes after review. Linux fake
service tests are not proof of macOS framework execution; macOS compile tests
are not proof of live recognition or on-device availability.

## Alternatives rejected for the first implementation

- Permission requests inside `start`: unchanged CUI starts capture immediately;
  this adds consent-time recording and a pending PCM queue. Explicit prepare
  avoids that architecture change.
- Treating `cancel` as synchronous callback shutdown: not a documented Apple
  contract; raw userdata could be freed before a late block runs.
- One reusable global mailbox: a late old block could affect a new session.
  Per-operation retained endpoints and bounded retirement prevent that alias.
- Unbounded dispatch/channel queues or per-callback heap strings: UI starvation
  becomes unbounded host-owned growth. Fixed payloads fail visibly instead.
- Reusing the SDL SPSC ring for recognition events: Apple permission/result
  callbacks may be concurrent; multiple producers need their own synchronization.
- A network fallback when on-device support is absent: contradicts the selected
  privacy contract and frozen macOS comparison. Return unavailable.
