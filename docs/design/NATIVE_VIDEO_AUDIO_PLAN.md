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
