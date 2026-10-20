# Optional SDL Speech input host

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

## Validation boundary

Standalone external CMake builds only existing CUI speech base code plus this
host fixture and optional SDL source, with strict warnings. Injected devices
check startup failure, synchronous delivery, PCM ordering, ring overflow,
error forwarding, wake failure, stop/cancel cleanup, restart rejection of stale
reports and shutdown rejection. Tests use deterministic synthetic PCM and no
SDL initialization/device open. A separate unbound SDL stream proves conversion.
Run CTest through a local private-Xvfb wrapper; native desktop windows are never
opened. Real capture is a separate explicit user action, never an automated
fixture. Manual OS consent, disconnect/reconnect, audio hardware and recognizer
behavior remain unvalidated. macOS/Windows compilation is deferred until those
platforms are exercised.

## Implemented independent slice

The signed design checkpoint precedes implementation. `host.h` exposes only
portable application-host types; `host.c` depends on existing CUI speech base
APIs and standard C11, and `host_sdl.c` keeps all SDL types private. Creation
allocates one fixed PCM ring and rejects platforms without lock-free atomic
indices/error/acceptance flags. Capacity is **samples**, not frames; leave room
for the recognizer's channel count. One additional sample slot disambiguates
empty/full. No per-batch host allocation occurs.

The private SDL callback accepts at most 8192 converted bytes per initial
callback snapshot and uses 256-sample stack chunks. This explicit guard can
fail high-rate/high-channel streams even when the ring is large enough. The
usual 16 kHz mono speech format is the intended starting point, not a claim
that every format accepted by CUI works with every device. Terminal callbacks
clear remaining SDL data. The first terminal error wins, even if wake also
fails. Provider close detaches SDL's put callback under the stream lock (which
waits for an existing callback), then destroys the stream/device. See
[SDL's callback lock contract](https://wiki.libsdl.org/SDL3/SDL_SetAudioStreamPutCallback).
No device opens in the factory or host constructor.

Build and run only these independent fixtures from the repository root:

```sh
cmake -S examples/speech_sdl_input -B /tmp/cui-speech-input-build -G Ninja
cmake --build /tmp/cui-speech-input-build
python scripts/test_local.py --build-dir /tmp/cui-speech-input-build
```

The build uses the installed SDL >= 3.2; it performs no download/install. CUI's
existing optional fetched SDL pin is documented in the design proposal; this
standalone build does not fetch dependencies. Verified here with SDL 3.4.18,
GCC 16.2, strict C11 and `-Wall -Wextra -Wpedantic -Werror`.
Four private-Xvfb CTest fixtures pass on Linux:

- Injected lifecycle: synchronous start delivery, ring wrap/order, whole-frame
  polling, malformed frames, full ring, device/wake errors, startup cleanup,
  CUI queue capacity failure, stop timeout, cancel/restart stale sinks and
  exactly-once close. Active-host destruction is rejected.
- Unbound SDL stream conversion: exact PCM identity, stereo downmix/resampling,
  counts and signal tolerance; no initialized audio subsystem.
- Injected SDL device operations: every init/open/resume/device-close operation
  is intercepted with linker wrapping; real unbound streams exercise the actual
  SDL callback, terminal drain and detach code. No real device operation runs.
- C11 worker producer: 4096 ordered samples traverse a wrapping fixed ring;
  recognizer delivery occurs on the owner thread; close joins the producer and
  rejects late delivery before host destruction. The threaded fixture has a
  10-second timeout to expose deadlocks.

The Linux-only linker-injection/thread fixtures are not registered on other
platforms; macOS/Windows compile, permissions and hardware remain deferred.
These tests establish neither recognition accuracy nor microphone operation.

## Application integration and manual capture boundary

An application supplies a real recognizer, caller-owned CUI state/workspace,
and a monotonic UI-loop timer. In the application's explicit audio-enabled
path, initialize SDL's audio subsystem before starting Speech; the SDL provider
returns UNSUPPORTED if audio is not initialized. Pass
`speech_input_host_provider(host)` as the input to `cui_speech_init`, where
`host` was created with `speech_input_sdl_device_ops()` and a bounded capacity.
Only an explicit user start action should call `cui_speech_start`; that is the
point that can open a microphone and trigger OS consent. Construction and
availability checks do not establish permission or prompt for it.

While active, arrange a short UI timer (for example 10 ms); each tick calls
`speech_input_host_poll(host, monotonic_seconds, budget_samples)` followed by
`cui_speech_pump(state, monotonic_seconds)`. Budget and CUI workspace must fit
the selected format and expected timer latency. Keep pumping during STOPPING.
An optional wake posts a nonblocking UI notification and returns its actual
success; the timer remains necessary for wake failure and stop deadlines.
UI callbacks must schedule later work instead of reentering poll/lifecycle.

Shutdown order is: cancel/dispose CUI state, destroy host (must succeed), then
terminate the application's SDL audio subsystem. The host must outlive the
state's provider callbacks even when inactive. An injected device implementation
must uphold serialized delivery and quiescent close just as the SDL backend
does. Never invoke a saved delivery callback after close returned; generation
checks cannot make a freed host pointer safe.

This slice does not wire SDL audio-device-removal events into application UI.
The application must handle those events and cancel the affected Speech state;
a stream that stops producing samples does not itself prove disconnection.
SDL open failures map to INPUT_ERROR because error strings do not supply a
portable permission/absent-device code. macOS usage descriptions/consent,
Windows privacy settings, Linux audio service/device access, removal recovery,
real recording and OS recognition require separate explicit manual validation.
