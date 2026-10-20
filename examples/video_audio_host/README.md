# Offline native-video audio controller foundation

This optional standalone C11 host implements bounded PCM buffering, a worker
with injected decoder operations, and a consumption ledger with an injected
output lifecycle. It has **no FFmpeg decoder, SDL output, device access, playback
UI or Core dependency**. Native VideoPlayer remains silent. The interfaces here
are example-local, not installed CUI public APIs. There is no rate conversion,
network loader, permission request or new external dependency.

The fixed format is interleaved stereo float32 at 48,000 frames/second. Input is
already normalized, finite and contiguous in that frame timebase. Decoder DATA
must report the exact next source frame and a nonzero count within the supplied
capacity. Other sample rates, channels, arbitrary PTS gaps, codec priming and
resampler-delay mapping require a future decoder transport; mismatches are
rejected, not guessed. At generation creation/seek, position is the requested
source-frame target; subsequent advancement requires consumption evidence.

## Consumption contract for review

`video_audio_render` copies PCM and pads starvation with explicit zero samples.
It assigns every accepted output frame a bounded ledger entry and returns a
`{generation, first_sequence, frames}` ticket. A render is **submission**, not
presentation or consumption. The transport must not submit a rejected render.
A full ledger rejects the whole render and leaves the destination/ticket
untouched. It cannot accumulate an unbounded queue while the owner is stalled.

`video_audio_consume` accepts an ordered contiguous prefix of outstanding output
frames. Partial acknowledgements are supported; duplicate, reordered, excessive
and old-generation acknowledgements reject atomically. Consumption of a media
entry advances position to that media frame's end. Consumption of a zero-fill
entry leaves position unchanged. The transport reports actual consumption, not
requested callback bytes, decoded frame count, submitted bytes, or elapsed UI
time. Exact accounting is proved for the injected transport; this is not a claim
that an existing SDL API exposes exact hardware presentation time.

`video_audio_poll` copies a coherent mutex-protected snapshot. Submission and
consumption sequences reset per generation; media/silence statistics are lifetime
counts. Silence has separate submitted and consumed counters. Source positions,
sequence arithmetic and lifetime media/silence counters reject overflow instead
of wrapping; rejection statistics saturate. EOF requires decoder EOF plus no
queued or unconsumed media. Trailing acknowledged silence cannot move the clock.
An output transport can remain active emitting silence after `ended`; the owner
must pause/stop it. There is no self-destruction or device lifecycle operation in
an output callback.

Pause synchronously quiesces output before freezing effective state. Pending
output and its ledger are retained for resume, so the transport must resume that
same order. Seek synchronously quiesces/discards output, increments generation,
clears PCM/ledger, changes target and schedules worker seek. Seek leaves playback
paused until explicit play. Late results from the previous worker generation
are discarded. Decoder interruption includes a generation ceiling and must be
sticky for that generation; a delayed old interrupt must never cancel a newer
seek/read. Decoder callbacks never receive or retain the host pointer.

All lifecycle commands run on the creating owner thread. PCM render/consumption
and snapshot polling may run on other threads while the host remains live. No
arbitrary callback runs under the host mutex. Critical sections have configured
frame-count bounds but use an ordinary C11 mutex: **hard realtime scheduling or
lock-free callbacks are not claimed**. A decoder AGAIN result retries after a
10 ms condition wait; this retry timer is unrelated to media time.

The injected output `stop` is the essential lifetime boundary: success proves all
entered callbacks have returned and no new callback can enter until `start`.
Discarding stop must also release retained host pointers and queued notifications
and report final actually consumed frames before returning. `destroy` refuses to
free the host when stop fails; callers retain a live host and can retry. After
successful quiescence it signals stop, interrupts decoder operations, joins the
worker, then frees storage. Never race unrelated public calls with destruction.
Decoder operations must cooperate promptly with interruption; a hostile/blocking
service that ignores this contract has no bounded shutdown guarantee.

A future physical output adapter must separately resolve device-pending frames,
latency, pause retention, discard semantics and safe detach. SDL callback demand
and `SDL_GetAudioStreamQueued()` are not sufficient proof of actual consumption.
No such adapter is implemented or approved by this foundation. Do not wire this
to SDL/FFmpeg or the existing video UI until that contract is reviewed.

## Memory and backpressure

For capacity C and decode chunk D (1 <= D <= C <= 65,536), allocations are one
2*C float PCM ring, one 2*D float scratch array, C output ledger entries and the
fixed host. The PCM ring and ledger are independently capped: at most C decoded
media frames plus C submitted output frames. Each ledger entry stores one media
frame index and a media/silence tag; its C padding is included via `sizeof`.
Allocation occurs only at create. The worker waits when PCM capacity is full;
render never waits for decoder data and ledger pressure rejects rather than
expands. A capacity of 12,000 represents 250 ms in each queue. The maximum allowed
capacity is not a latency recommendation. Decoder/output implementation memory
and OS buffers are outside this host-owned bound.

## Reproduce offline tests

No package install or download is performed. The project builds only this host
and its fixture, without configuring the framework or opening any device/window.
On Linux use the repository's invisible-display wrapper even for these tests.

```sh
cmake -S examples/video_audio_host -B "$HOME/.cache/cui-validation/video-audio-controller" -G Ninja
cmake --build "$HOME/.cache/cui-validation/video-audio-controller"
python scripts/test_local.py --build-dir "$HOME/.cache/cui-validation/video-audio-controller" -V
```

For ASan/UBSan, use a separate directory and add
`-DVIDEO_AUDIO_SANITIZE=ON` to configure. The standalone project uses strict C11,
`-Wall -Wextra -Wpedantic -Werror` and assertions enabled (MSVC: `/W4 /WX`).

Six deterministic fixtures cover known signed stereo samples/counts, submission
versus consumption, partial acknowledgement, silence starvation, EOF tail,
pause/resume pending output, queue/ledger capacity, reordered/duplicate/stale
acknowledgements, repeated seek/restart, ring wrap, malformed/nonfinite decoder
results, explicit decoder/seek errors, start/stop failure, wrong-owner commands,
blocked read and seek cancellation, successful stale decoder data rejection,
callback quiescence before free, and a
257-media-frame stream with concurrent decoding/output/polling. No wall time is
used to infer playback position. Bounded waits only detect deadlocks/starvation.

## Remaining implementation

Common container A/V timestamp normalization, real audio decoding/resampling,
video demux scheduling, physical output, physical presentation clock/latency,
owner-thread CUI integration, audible pause/seek behavior, device loss, sample
rate changes and platform CI remain deferred. Fixed rate 1 is the only intended
initial playback mode; there is no rate API here. Tests do not establish sound
output or full native VideoPlayer parity. The Linux GCC and Clang native CI jobs
build this optional example separately with ASan/UBSan and run all six offline
cases through the invisible-display wrapper.

Validation of this checkpoint: all six focused CTest cases passed through
`scripts/test_local.py` in normal and ASan/UBSan builds on Linux (GCC 16.2.1).
clang-format 23.1.2 and diff checks passed. No other platform compile, hardware
latency or physical output result is implied.

## Offline mutex observation fixture

`bench_mutex.c` is a separate Unix-only fixture; controller code is unchanged.
It measures render-plus-consume pairs using CLOCK_MONOTONIC while the decoder
worker contends for the same mutex. Configure a Release build, build normally,
then run `scripts/test_local.py --build-dir BUILD -R video_audio_mutex_observation -V`.
The fixture checks PCM classes and consumption counters but sets no deadline
pass threshold. It is registered with the `benchmark` CTest label. It neither
opens output nor simulates a physical-device scheduler.

One Linux x86_64 / GCC 16.2.1 Release observation (4,000 pairs, C=12,000,
256-frame output blocks; durations in microseconds):

| Decoder chunk | p50 | p95 | p99 | Observed max | Pairs above 5.33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 256 | 6.211 | 19.160 | 31.041 | 427.163 | 0 |
| 4,096 | 4.655 | 19.302 | 31.030 | 105.800 | 0 |

The tight loop is deliberately not paced at 48 kHz: it consumed 1,017,536 / 998,592
media frames respectively, with 6,464 / 25,408 underrun-padding frames. Counter
checks ensure that padding did not advance media time. These are observations,
not an absence-of-dropouts claim. All seven CTest cases (six foundation fixtures
plus observation) passed in this Release build and a separate ASan/UBSan build.
The instrumented probe's observed maxima were 2.134 / 1.913 ms; instrumentation
and scheduling differ, so those values are not directly a performance comparison.
No hard realtime, hardware-latency or worst-case bound follows from any run.

The next real-adapter proposal, including the explicitly named SDL software-mixer
clock and its remaining acoustic A/V limits, is in
[the design checkpoint](../../docs/design/NATIVE_VIDEO_AUDIO_PLAN.md). Actual SDL
queue-drain/format/quiescence fixtures must pass before that adapter is added.
