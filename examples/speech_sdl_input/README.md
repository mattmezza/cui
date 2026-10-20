# Optional SDL Speech input host (design checkpoint)

This independent application-host example feeds existing `cui_speech_input`.
It is not a recognizer or a Core/Base API change. Linux still needs an
application recognizer. No automated test opens an audio device.

## Bounded ownership decision

Use a fixed-capacity single-producer/single-consumer PCM ring, not a timer-only
SDL pull queue. Device callbacks are serialized producers; the UI thread polls
copied samples into `cui_speech_report_audio`, then calls `cui_speech_pump`.
C11 acquire/release indices publish complete PCM frames. Only the callback
writes the producer index; only UI poll writes the consumer index. The callback
never enters CUI, calls lifecycle methods, or retains the sink. A terminal
atomic error is separate from the ring so overflow can always be reported.
The fixed bound applies to host-owned PCM, not all SDL/OS capture buffers.

SDL's private callback drains converted PCM into a bounded scratch array.
If a batch exceeds available ring space it marks overflow; subsequent callbacks
clear SDL pending data until owner-thread shutdown. SDL/driver scheduling and
internal buffers are outside this example's memory bound. UI starvation causes
a visible terminal failure rather than silent speech loss or unbounded host
queue growth. The host must poll while a session is active and enforce CUI's
STOPPING deadline even without a wake. This example does not promise hard
real-time behavior or a bound on every allocation in SDL/the OS.

Optional callback wake is a thread-safe, nonblocking notification only. A false
return marks input failure; the UI's active-session timer must still poll to
observe wake failure. Wakes can be redundant and late; they carry no sink/state
pointer. No wake callback may synchronously poll or close the host. The example
uses no wake coalescing, avoiding a missed-wake acknowledgement race.

## Lifecycle and injection contract

The host allocation and provider live on one owner/UI thread. Device operations
are injected behind a portable C11 header. `open` receives requested signed
interleaved PCM16 format, a sample/error delivery callback and user pointer.
It must serialize callbacks, including any callbacks during `open`. On failure,
`open` releases all partial resources and returns no token. On success it
returns exactly one token. The provider's close invalidates its accepting flag,
closes that token once, and waits for callbacks to quiesce before resetting ring
indices. It must not wait for callbacks that are waiting for UI dispatch.

The host retains a CUI sink only until provider close. CUI closes capture on
stop, cancel, startup failure and dispose. Before freeing the host, first
cancel/dispose the CUI state so no provider callback can subsequently use it.
Destroying a still-active host is rejected. Closing the device quiesces old
callbacks before a new session can reuse the host; generation-tagged CUI sinks
also reject previous-session reports while the state allocation remains alive.
The raw sink does not permit reporting after freeing state/workspace.

Owner polling snapshots whole PCM frames into local scratch before reporting.
On ring overflow/device error/wake failure it reports a terminal error and
lets `cui_speech_pump` close capture/recognition. On CUI queue capacity failure,
stop polling and let pump surface its existing capacity terminal error. Never
claim that a dropped batch was recognized. Poll is forbidden during provider
callbacks and must not run from event/wake/audio callbacks.

## Planned proof and limits

Standalone external CMake builds only existing CUI speech base code plus this
host fixture and optional SDL source, with strict warnings. Injected devices
will check startup failure, synchronous delivery, PCM ordering, ring overflow,
error forwarding, wake failure, stop/cancel cleanup, restart rejection of stale
reports and shutdown rejection. Tests use deterministic synthetic PCM and no
SDL initialization/device open. A separate unbound SDL stream proves conversion.
Run CTest through a local private-Xvfb wrapper; native desktop windows are never
opened. Real capture is a separate explicit user action, never an automated
fixture. Manual OS consent, disconnect/reconnect, audio hardware and recognizer
behavior remain unvalidated. macOS/Windows compilation is deferred until those
platforms are exercised.
