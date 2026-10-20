# Optional native video audio host: design checkpoint

Status: proposal only. Native playback is still explicitly silent. This document
adds no implementation or parity claim. Audit baseline: video integration
`9d4914830f29bcdd8b12517f21841fbdae17f631`; browser audio proof does not establish
native audio support.

## Existing implementation and the necessary boundary

`examples/video_player/decoder.c` owns one local-file FFmpeg demuxer on its C11
worker. It selects only video, discards other packets, converts to RGBA, and
blocks when its three reusable frame slots are occupied. Generation checks and
explicit frame leases protect pixels through renderer consumption. The UI in
`main.c` advances the silent `cui_video` timeline using
`cui_sdl3_frame_delta()`. Its fixture explicitly uses `-an`. Neither that fixture
nor the native screenshot tests exercise audio.

Audio therefore requires a coordinated host implementation, not merely opening
an output device. Two readers must never call `av_read_frame()` on one format
context. Blocking video publication must not prevent feeding audio. Separately
subtracting each stream's start time would destroy intentional A/V offsets.

Keep Core, Base, components and their existing public headers unchanged. Add an
optional application host alongside this example, with platform-free C11 status,
command and PCM types; FFmpeg and SDL types stay in private implementation files.
The existing timeline remains host-driven. No framework audio dependency, codec,
thread pool, network loader, or device initialization is introduced.

The current exact audited FFmpeg dependencies are libavformat 63.1.102,
libavcodec 63.1.102, libavutil 61.1.102 and libswscale 10.1.102. The installed
libswresample 7.1.102 is a practical optional conversion dependency, but is not
currently a dependency of the example. A production slice must explicitly pin
and audit it in the external harness, recording version, configuration and
license with the other FFmpeg libraries. The locally audited FFmpeg build is
GPLv3-or-later; MIT host sources do not change linked-library obligations. Do not
silently install or fetch another codec stack. SDL's repository fallback is
pinned to `7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`; system SDL currently satisfies
an SDL >=3.2 requirement rather than that exact source revision.

`cui_sdl3_create()` initializes VIDEO only. The optional host must separately
balance AUDIO subsystem ownership and close audio before destroying the window
host. SDL runner callbacks and all CUI calls remain on the owner/UI thread.
The runner's idle waits reset its frame delta, making that delta unsuitable as
an audio master clock. Window actions do not supply audio controls. Existing
VideoPlayer requests expose play, seek, skip, fullscreen and rate, but no volume
or mute request; host configuration can supply initial gain without inventing
framework capabilities.

## Minimum implementation decision

Start with local files, one selected audio stream, rate exactly 1, and explicitly
supported PCM layouts converted on the decode worker to interleaved float32,
48 kHz stereo. Reject unsupported layouts, invalid timestamps, discontinuities
and oversized data with actionable status. Files without audio may retain the
existing explicitly silent clock. Failure to open audio for a file containing
an audio track must fail audio playback or require an explicit user-selected
silent fallback; it must not report successful sound output.

Use one demux/decode worker initially. It owns the format, both codec contexts,
resampler and conversion scratch. Drain each codec correctly around send/receive
EAGAIN and EOF. Publish video without blocking audio: replace only unleased
queued video frames, dropping superseded video as needed. Never overwrite a
leased frame. If no video slot is free, skip conversion/publication of that
frame; continue draining codecs. PCM publication may wait on a bounded ring,
with stop/seek commands waking that wait. The audio callback never waits for the
worker. There is no unbounded demux packet spool: retain only the current packet
and codec-required references, reject a packet above a documented byte limit,
and bound scratch dimensions/sample counts before allocation. FFmpeg internal
allocations and device/OS buffers are outside the host-owned memory bound.

Suggested initial host limits: 250 ms PCM (12,000 stereo float frames, 96,000
bytes), three existing video slots, one packet capped at 8 MiB, and a configured
maximum decoded audio frame/sample count. Exact limits become tested constants,
not tunable promises without validation. Deeply interleaved or malformed media
can exceed these limits or starve output; expose buffering and ultimately a
bounded-progress failure rather than grow a packet queue or discard audio
packets. A later independent demux/audio/video worker design may improve this
case, but requires byte-and-count-bounded queues and command cancellation at
every wait. It is unnecessary for the initial local-file slice.

The output callback copies available PCM and fills any remainder with silence.
It performs no allocation, decoding, CUI call, blocking wait, or host destruction.
It maintains a bounded ledger of generation, media-frame ranges and inserted
silence. A small lock protecting ring/ledger operations is permitted only after
measuring bounded critical sections; no FFmpeg, UI or device calls occur under
that lock. UI commands use acknowledged serials and generations, not raw pointers
passed through callback events.

## Clock and synchronization contract

Use a common source timestamp origin with rational rescaling. Preserve audio and
video start offsets; account for codec priming, sample trimming, resampler delay
and final drain. The first implementation must reject cases it cannot map rather
than silently reset each stream to zero. `swr_get_delay()` and actual converted
sample counts determine bounded conversion output and timestamp bookkeeping.

The master is output progress through the media-frame ledger, never decoded or
submitted sample count and never UI frame delta. An injectable output transport
reports consumed frames and queued latency; deterministic tests provide exact
consumption. For SDL, distinguish frames pulled into SDL processing from frames
physically heard. SDL 3.2 exposes queued input bytes and device buffer size, not
an exact DAC presentation timestamp. Callback byte requests can overestimate
actual demand. Do not use those requests as a hardware clock or equate
`SDL_GetAudioStreamQueued()` with converted/device-pending bytes.

The first SDL transport must document its software-consumption clock and bounded
latency estimate, keep SDL staging bounded, and measure callback-consumed PCM
rather than requested bytes. It cannot claim sample-exact audible A/V sync.
If the chosen SDL stream mode cannot expose sufficient actual consumption to
maintain this contract, stop at the injected transport slice and require a
separate output-clock design review. No fallback to a submission clock. Exact
hardware presentation timing is a later backend-specific extension.

On owner-thread polls, copy a coherent clock snapshot, advance the existing
`cui_video` by the nonnegative media-time difference, and select the latest video
frame due at that clock (with a stated small scheduling tolerance). Never count
inserted underrun silence as media progress. Enter BUFFERING, freeze at the last
consumed media sample, prefill, then rebase after restart. Video may drop late
frames but may not pull audio forward. Active playback requests regular owner
polls independently of UI animation. Use an owner-controlled bounded timer/frame
schedule first; a future SDL user-event wake requires runner dispatch proof,
coalescing, failure fallback and shutdown lifetime tests. Worker callbacks must
never invalidate CUI directly.

## Controls, generations and destruction

- Play starts only after format negotiation and prefill. Pause pauses output,
  acknowledges its effective state, and freezes the consumption clock; samples
  already in hardware impose a documented audible latency. Resume rebases from
  retained media position. UI state must not race ahead of command acknowledgement.
- Seek increments the shared generation and serial, pauses/quiesces output,
  clears pending PCM/SDL staging, flushes both codecs and resampler, seeks the
  common source, then trims decoded audio to the requested sample boundary.
  Discard stale worker output and clock snapshots. Keep old video leases valid
  until released, but never render them in the new generation. A video preview
  timestamp must not independently rebase the audio clock. Paused seek stays
  paused. Restart/loop use the same transaction.
- Remove RATE capability and reject non-1 rate requests in this slice, including
  the example's current 1x/2x toggle. Pitch-preserving rate conversion is deferred.
- EOF means demux EOF, both codecs and resampler drained, all media PCM consumed,
  and the documented output tail drained. Video EOF or container duration alone
  cannot end audio. Keep the final video frame leased as today. Distinguish EOF,
  underrun and decode/output failure.
- Close first rejects commands and detaches notifications, signals stop and wakes
  all producer waits. Quiesce/detach the output callback before freeing its ring
  or userdata; pause alone is not proof of callback quiescence. Join the worker
  without holding a callback/queue lock, release renderer leases on the owner,
  destroy streams/device, free codecs/queues and balance AUDIO initialization.
  The exact SDL stream-lock/detach ordering requires an implementation-level
  review and test. No raw host pointer survives close in an event or callback.
  Output loss, seek, failure and close must all unblock bounded producer waits.
  AVIO interruption observes shutdown; bounded seek latency for arbitrary local
  formats remains unproven and is not advertised.

## Reviewable slices and validation gates

1. Add an optional platform-free controller/clock/ring with injected decoder and
   output transport. Strict C11 warnings and deterministic threaded tests cover
   actual sample values and frame counts, wrap/overflow, partial callback reads,
   starvation/silence clock freeze, delayed UI polls, A/V offsets, pause/resume,
   seek trimming, stale generations, EOF tail, output loss and close while every
   producer wait is occupied. Test callback quiescence and late notification
   rejection after host free. This slice alone is not native audio support.
2. Add real FFmpeg audio decoding/resampling and deterministic muxed fixtures:
   known stereo tone/impulses with red/blue video markers, different source sample
   rates, nonzero offsets, audio shorter/longer than video, and no-audio input.
   Assert decoded PCM counts, channel content and tolerances, timestamp mapping,
   seek content and resampler drain. Include deliberately excessive packet/frame
   sizes and invalid timestamps. Existing `-an` fixtures remain video-only proof.
3. Add SDL output and example UI integration only after the clock gate above.
   Run `SDL_AUDIODRIVER=dummy` with an explicitly selected nonphysical device and
   verify submitted/consumed ledger counts, pause, clear, restart, EOF and teardown.
   Dummy timing is integration evidence, not proof of physical sound or latency.
   Linux native window tests run only through `scripts/test_local.py` with its
   private Xvfb; never fall back to a desktop display. Prefer the injected sink
   for exact timing assertions; dummy driver tests use bounded timeouts, not
   brittle wall-time equality. No automated real speaker/device activation.

Use a standalone external CMake harness building only needed example/library
and test targets, pinned audited dependencies, strict warnings, and the native
wrapper's CTest result. Add ASan/UBSan and concurrency stress where supported.
No broad rebuild or package/skill regeneration is needed for this proposal;
production integration must update corresponding metadata and consumer guidance
through the integrator. macOS and Windows need actual focused build/test proof;
Linux dummy output cannot establish their device behavior. Manual opt-in speaker
checks must later assess audible A/V alignment, device switching, latency,
Bluetooth and long playback. Hardware timing, rate changes, volume UI, network
sources, multitrack selection, edit-list coverage and universal codec support
remain explicit gaps.

## Primary API references

- [SDL audio callback contract](https://wiki.libsdl.org/SDL3/SDL_AudioStreamCallback):
  callback thread/stream lock, approximate requested counts, no callback destruction.
- [SDL queued input bytes](https://wiki.libsdl.org/SDL3/SDL_GetAudioStreamQueued):
  input queue accounting differs from converted output and is not a hard queue cap.
- [SDL device format/buffer size](https://wiki.libsdl.org/SDL3/SDL_GetAudioDeviceFormat)
  and [pause](https://wiki.libsdl.org/SDL3/SDL_PauseAudioStreamDevice).
- FFmpeg's installed pinned `libswresample/swresample.h` documents conversion
  bounds, `swr_get_delay`, timestamp conversion and draining; implementations must
  validate against these pinned headers rather than assume newer API behavior.

## Implemented independent foundation; transports still proposed

`examples/video_audio_host` now implements the first injected-only controller
slice without modifying existing decoder/UI/Core code. It owns a bounded PCM
ring and a separately bounded per-output-frame media/silence ledger, one decoder
worker, generation-aware sticky cancellation and synchronous output-quiescence
requirements. Render/submission does not advance the clock. Ordered actual
consumption acknowledgements advance source-frame position only for media;
silence, duplicates, reordering and stale generations cannot advance it.

The example README/header specify the consumption and lifetime contract for
review. In particular output stop must prove no active/future callbacks and
release host references on discard; failed stop leaves storage allocated. Seek
is paused, asynchronous worker work with an explicit generation change, not a
claim of sample-accurate container seek. Input is already normalized contiguous
48 kHz stereo float PCM; timestamps, priming and sample conversion remain future
transport work. Ordinary bounded mutex sections are used, not a hard realtime
or lock-free callback guarantee. Host-owned allocations and both queue bounds
are explicit; external service/device allocations are excluded.

The standalone strict-C11 fixture uses injected decoder/output operations only.
No SDL/FFmpeg transport, physical device, UI integration or audible native audio
support has been added. Those slices remain gated on review of actual
consumption accounting and platform-specific quiescence/latency evidence.

Foundation validation: six focused offline cases passed through the private
native-test wrapper in normal and ASan/UBSan Linux builds. They include exact
known PCM and consumption counts, independent queue exhaustion, stale returned
DATA after seek cancellation, blocked read/seek shutdown, output callback join
before host free and concurrent stream accounting. This validates the injected
contract only; it supplies no evidence of a physical output consumption clock.

## Real-adapter feasibility checkpoint (no adapter implementation)

Audit after foundation `c63af6de`: installed SDL 3.4.18, FFmpeg n9.0.2,
libavformat/libavcodec 63.1.102, libavutil 61.1.102, libswscale 10.1.102 and
libswresample 7.1.102. Inspected existing headers and the SDL fallback source at
`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`. No device was opened. The only added
executable is an offline mutex observation fixture; controller production code
is unchanged. The following real transport design still requires review.

### Three different boundaries

| Boundary | Evidence available | Meaning and limit |
| --- | --- | --- |
| Host/SDL submission | Successful host render ticket and `SDL_PutAudioStreamData` | Bytes accepted into a software queue; not playback progress. |
| SDL stream input consumed by mixing | Successful-put byte ledger minus remaining input queue, under the restricted invariant below | Actual asynchronous removal of this stream's PCM for SDL mixing; useful software clock, not device acceptance. |
| Backend accepted / physically presented / acoustically audible | No equivalent portable SDL timestamp/count API established in this audit | Backend buffering, OS routing, DAC, speakers and propagation are outside the proven clock. These stages are themselves different. |

The recommended first SDL clock is explicitly **SDL software-mixer input
consumption**. Native audio playback need not wait for a universal hardware
presentation timestamp, provided this boundary, queue bounds and latency limits
are visible. Do not silently equate the controller's generic consumption wording
with acoustic truth. The adapter's example-local documentation and UI host status
must name its chosen clock boundary before integration.

Source evidence: the pinned stream implementation invokes its get callback
before removing input; it returns the bytes actually retrieved to its caller.
`SDL_GetAudioStreamQueued` reads remaining input bytes. Equal-rate operation with
unity frequency ratio and zero resample offset bypasses its resampler. These
facts permit a restricted drain calculation, not a general resampling clock.
See [pinned SDL stream implementation](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/audio/SDL_audiocvt.c).
The pinned device loop drains bound streams, invokes postmix, then calls the
backend `PlayDevice`; postmix is consequently before backend acceptance.
See [pinned SDL device loop](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/audio/SDL_audio.c).

Neither callback request sizes nor postmix buffer length prove that this
particular stream supplied all those frames. SDL can mix automatic silence.
[Postmix documentation](https://wiki.libsdl.org/SDL3/SDL_SetAudioPostmixCallback)
also permits changing channel counts/rates and describes synchronous callback
replacement. [Get-callback documentation](https://wiki.libsdl.org/SDL3/SDL_SetAudioStreamGetCallback)
provides a refill opportunity, not a post-play acknowledgement.

### Restricted same-format software-drain adapter

Use one private logical playback device and one privately owned bound stream.
Normalize decoder output to native float32 stereo 48 kHz on the worker. Require
stream source and destination to remain float32 stereo 48 kHz, unity frequency
ratio, default channel maps and no format changes for the whole generation.
Check the bound destination and postmix spec, not just the requested open spec.
An unsupported obtained format or hot device change stops playback with an
explicit error; do not continue with an unproved sample mapping. SDL can still
convert final float values to a physical integer format without changing the
selected software frame-count boundary; no physical-clock claim follows.

For a generation, maintain checked successful-put bytes P, remaining queued
input bytes Q, and previously acknowledged frames A. All values are private to
this stream and measured coherently with its stream lock:

```
frame_bytes = 2 * sizeof(float) = 8
software_drained_frames = (P - Q) / frame_bytes
new_consumption = software_drained_frames - A
```

Only acknowledge this ordered prefix, never all submitted frames or callback
request sizes. Require nonnegative/aligned Q, Q <= P, monotonic drain, and a
prefix within outstanding controller tickets. Cap both software queues well
below SDL's int-count clamp; C=12,000 means at most 96,000 input bytes. Prove the
identity on each admitted SDL build using unbound-stream actual reads, then a
private dummy-output postmix fixture. No resampling, format-track changes,
foreign puts/gets/clear calls or silent queue reset may occur inside this identity.
A copied frame value of zero can be valid decoded media; only controller-added
underrun padding is tagged nonmedia and excluded from media-time advancement.

A refill callback may submit a bounded aligned chunk when requested. Count P
only after a successful put. Postmix observes actual remaining queue after the
mixer's read and acknowledges newly drained entries. Do not synthesize
consumption from postmix silence. Input drain can include frames in SDL's current
mix buffer that are never accepted if the backend fails afterward; status must
say software consumption rather than successful hardware playback.

There is a transaction limitation in the existing foundation: render reserves a
ticket before SDL put can fail, and no unsubmit API exists. On put failure,
immediately stop feeding and surface an adapter failure on owner poll. Quiesce,
discard and require seek/reset from the last acknowledged source position or
destroy/recreate before retry. Never acknowledge the failed put or send later
chunks across its unsubmitted sequence gap. No Core schema change is necessary;
minimal example integration can keep this failure in host-owned state.

### Pause, seek, flush, close and locking

Open/configure while paused, validate format, then install callbacks and resume
only after the host is ready. A failed start must leave no callbacks running;
resume/setup failure branches need injected coverage before claiming the
foundation start contract is met. All SDL operations run outside the controller
mutex. Callback paths take stream lock then controller lock, never call CUI,
never wait for owner/UI, and never change device/callback lifecycle themselves.
The owner must never acquire those locks in the reverse order.

The proposed stop sequence is: pause the logical device, detach postmix with
`SDL_SetAudioPostmixCallback(..., NULL, NULL)`, detach get callback using its
stream-serialized setter, then observe a final coherent input-drain prefix.
The first setter waits out the current device iteration; the stream setter
serializes with an active get callback. Report final consumed frames while the
host is still logically playing, then return success to freeze owner-visible
state. Failure to prove either detach keeps the host allocated, as the existing
foundation requires. Verify exact order against both supported builds and
injected failures; pause alone is not the quiescence proof.

For pause, retain stream queue and outstanding tickets for ordered resume. For
seek/close, after detachment acknowledge only the final drain, then clear the
stream and discard mappings. Queue reduction caused by clear is **discard**, not
consumption. `SDL_ClearAudioStream` does not retract already submitted hardware
buffers: see [clear contract](https://wiki.libsdl.org/SDL3/SDL_ClearAudioStream).
Only then may the host increment generation/reset queues or free storage. Remove
all retained raw host/wake pointers and queued notifications before success.
No old postmix/get event may acknowledge a new generation. A small adapter
mailbox can report errors; owner polling can initially provide all UI scheduling.

The foundation ended flag means all media reached the chosen software boundary.
It does not prove the speakers finished. Likewise pause/seek can leave a hardware
or OS tail from the previous position. Application queue latency has a known
capacity bound; total audible latency does not. Device buffer size/frequency is
only a latency component, not a promise about OS/backend queues. Start with no
acoustic compensation promise; a later optional calibrated video delay can use
an explicitly estimated latency. There is no hard A/V error bound under UI/OS
stalls. Report synchronization against the software clock and defer measured
speaker/display alignment to explicit manual platform checks.

### FFmpeg service: bounded asynchronous conversion and seek

Keep one worker as sole owner of the format, both codec contexts, frames,
packets, scaler and resampler. Its injected audio read can demux/decode video
encountered on the way to audio and publish into a bounded video lease queue.
Video publication drops superseded unleased frames rather than blocking audio.
Never add a second reader to the same `AVFormatContext`. Deep interleave and
per-packet/decoded-frame limits remain explicit rejection/buffering cases.

Use `avcodec_send_packet`/`avcodec_receive_frame` with EAGAIN drain discipline,
then null-packet EOF drain. The
[FFmpeg 9 decoding contract](https://ffmpeg.org/doxygen/9.0/group__lavc__decoding.html)
defines those operations; it does not make concurrent seek/flush safe. An owner
seek publishes a serial/generation and atomic cancellation ceiling only. AVIO
interrupt checks those atomics; it never calls CUI or touches a context being
mutated by another thread. Old results, including successful DATA, remain
quarantined by generation. The first implementation can reopen the local file
on the worker for each seek to avoid relying on demuxer-specific recovery from
an interrupted AVIO operation. This adds I/O and has no universal seek-latency
bound. Preserve file-only protocol restrictions and bounded copied paths.

Maintain one rational container origin for both streams. Decode preroll, warm
the resampler, map output frames to that origin, then trim converted output to
the requested 48 kHz target before returning contiguous DATA. Do not independently
zero each stream or reset source time at each converted chunk. Gaps, negative
preroll and fractional-rate rounding need actual muxed impulse/video-marker
fixtures. Never subtract a guessed fixed resampler delay from every timestamp.

Proposed initial limits: mono/stereo input, fixed rate per stream (8-192 kHz),
no midstream layout/rate changes, decoded frames <=8,192 input samples, one
bounded converted spill buffer <=65,536 stereo output frames. Query
`swr_get_out_samples` immediately before conversion, reject a bound beyond that
buffer, use returned sample counts, and hand back chunks no larger than the
controller's D. Do not repeatedly feed input into insufficient output space;
that can grow internal buffered conversion data. `swr_get_delay`, checked
`av_rescale_rnd`/`av_rescale_delta` and optional `swr_next_pts` provide accounting
primitives, not a complete timestamp policy. In particular `swr_next_pts` uses
1/(input_rate*output_rate) units; automatic timestamp compensation must stay off
for rate 1. Any timestamp operation that changes SWR state requires recomputing
its output-space bound. Drain codec and converter at EOF; recreate/reset both
for seek. The pinned installed `libswresample/swresample.h` is authoritative;
see also the [FFmpeg 9 resampler API](https://ffmpeg.org/doxygen/9.0/group__lswr.html).

Start the actual decoder proof with lossless PCM audio muxed with the existing
lossless video markers, then 44.1-to-48 kHz conversion/seek impulse fixtures.
Do not claim AAC/Opus priming parity yet. The pinned codec header's
`AV_CODEC_FLAG2_SKIP_MANUAL` exports skip metadata instead of automatic skipping;
manual plus automatic trimming would double-skip. A first PCM-only subset can
reject nonzero codec padding/preroll and defer compressed-audio trimming until
fixtures establish the selected mode. Missing/unsupported timestamp or edit-list
cases must fail explicitly instead of returning invented contiguous timestamps.
`avformat_seek_file` uses the selected stream timebase (or AV_TIME_BASE for -1)
and ignores AVSEEK_FLAG_BACKWARD; choose min/target/max deliberately and decode
forward rather than assume that flag establishes an exact landing sample.

### Smallest real integration sequence and pins

1. Implement the FFmpeg service behind the injected interface and prove actual
   decoded/resampled PCM offline against the controller. No output device yet.
2. Prove same-format SDL queue-drain arithmetic with actual unbound streams,
   then a dummy logical output's postmix callback. Cover zero-fill versus media
   zeros, partial reads, pause retention, clear-as-discard, failed put/setup,
   generation-stale callbacks, stop failure, format mismatch and teardown.
3. Only after those proofs, add an optional example combining the shared demux
   service and SDL output. Owner polls source-frame position, converts to seconds
   at 48 kHz, advances the existing video model by coherent deltas and acquires
   due video leases. Seek must coordinate video/model/audio generations; any
   partial failure leaves the whole example paused/failed. Preserve leased
   pixels until renderer release. While playing/buffering, owner schedules
   frames independently of UI input; callbacks do not call CUI. Rate stays 1 and
   RATE capability is omitted. No-audio tracks retain the explicit silent mode;
   audio output failure cannot silently become successful sound playback.

Reuse existing exact FFmpeg pins and add explicit optional libswresample
`=7.1.102` with version/buildconf/license audit. The observed n9.0.2 package has
`--enable-gpl --enable-version3`; its recorded GPLv3-or-later linked obligations
remain unchanged. Record the entire linked configuration rather than infer LGPL
from the library name. SDL remains zlib-licensed, with existing fallback commit
pin above. Installed SDL 3.4.18 is an audited system version, not proof that every
`>=3.2` build behaves identically. Admit only tested builds for the drain adapter;
retain a runtime version/configuration record and require the same queue probes
when changing that set. No new dependencies enter Core, and no dependency is
installed by fixtures. macOS/Windows native compile/dummy proof and physical
playback checks remain separate prerequisites for platform claims.

### Mutex observation, not a realtime guarantee

The new Unix-only `bench_mutex.c` uses injected services and CLOCK_MONOTONIC to
measure 4,000 render-plus-consume pairs of 256 frames, with a contending decoder
worker, C=12,000 and D=256/4,096. It validates PCM classes and actual counter
identity while recording p50/p95/p99/max and observations exceeding the 5.33 ms
block duration. There is no deadline pass threshold. It measures controller
calls, not SDL callback duration, device latency or a worst-case scheduler bound.
Worker critical sections include finite validation/copy loops; the callback uses
an ordinary C11 mutex, so priority inversion and scheduling delays remain risks.

Run it through the private test wrapper using the standalone Release build:
`python scripts/test_local.py --build-dir BUILD -R video_audio_mutex_observation -V`.
The example README records the observed run. Future SDL dummy tests must also
measure the complete get/postmix path under decoder contention, queue pressure
and UI stalls; move validation/copies or redesign synchronization if measured
budget risk is unacceptable. Neither successful sanitizer tests nor a fast
microbenchmark establishes hard realtime behavior.

Alternatives retained for review: immediate acknowledgement after put is a
submission clock and is rejected; using only postmix buffer length incorrectly
counts SDL-generated silence and other streams; an unbound conversion stream's
manual read is valuable conversion proof but is not real device progression;
platform-specific hardware presentation timing could improve latency estimates
but would add another optional backend and platform validation scope. The
restricted bound-stream software-drain clock is the minimum useful native
playback route, with honest latency limits, rather than a reason to block native
sound until universal DAC timestamps exist.

Checkpoint validation: Release and ASan/UBSan standalone wrapper runs passed
7/7 (six controller cases plus the new observation fixture). The Release probe
recorded p99 render/consume pair durations about 31 microseconds and observed
maxima 427/106 microseconds for D=256/4,096 on this Linux GCC host. The unpaced
loop also produced underrun padding, which was separately counted and did not
advance media time. Full SDL callback timing and hardware playback are untested.
