# Optional SDL software-drain output proof

This standalone transport supplies `video_audio_output` to the unchanged audio
controller. It is not hooked to the video UI or FFmpeg service. Its clock is
**SDL software-mixer input consumption**, not a DAC, backend-accepted, hardware
presentation or acoustic clock. Native video sound support remains unimplemented.

```sh
cmake -S examples/video_audio_sdl -B build-audio-sdl -G Ninja
cmake --build build-audio-sdl
python scripts/test_local.py --build-dir build-audio-sdl
```

For ASan/UBSan repeat in a separate directory with `-DVIDEO_AUDIO_SANITIZE=ON`.
The existing SDL dependency is pinned to pkg-config/runtime 3.4.18 for this
restricted proof; no download/install/fallback or Core dependency is introduced.
SDL is zlib licensed. Admitting another SDL build requires rerunning the actual
stream/dummy probes. The real tests forcibly select `SDL_AUDIODRIVER=dummy`
before SDL initialization; no physical device, speakers, windows or network.

## Ownership, pumping and accounting

Initialize SDL audio explicitly in the application, open `video_sdl_open`, use its
interface in the controller config, and call `video_sdl_pump` on the owner thread
regularly while playing. Pump/snapshot/lifecycle are owner-thread only. Controller
decode remains on its own bounded worker. SDL receives **NULL callbacks and NULL
userdata**: no OS callback captures a controller, sink or application pointer.
No callback detach race or retired callback leases exist in this design.

Each pump locks the private stream, validates both stream formats (48 kHz,
native-endian F32, stereo) and unity frequency ratio, queries remaining input
bytes, acknowledges the newly drained ordered ticket prefix, then renders and
successfully puts at most one bounded chunk. Only successful puts enter the
private ledger. The stream lock excludes concurrent mixer drain across the
queue observation/put transaction; host render/consume run under this lock.
Nothing holds a controller lock while calling the output lifecycle callbacks.
The decoder never acquires the stream lock. Device pause/resume/close always run
**outside** the stream lock, avoiding SDL's device-before-stream lock inversion.
This uses ordinary locks on the owner, with no hard realtime/deadline claim.

For a generation, successful submitted bytes minus exact queued input bytes is
the consumed software prefix. Query errors, non-multiples of 8 bytes, counts
larger than outstanding successful puts, format changes or invalid ticket
acknowledgements fault playback. It is not valid to use converted-byte counts,
callback demand, elapsed wall time or a put itself as consumption. Ledger tickets
remain ordered even across partial drains. Controller-tagged underrun padding
does not advance media time; valid decoded PCM zeros do.

Max pending frames and one-pump chunk size are explicit (1..65536). Chunk must
also fit the controller capacity, and the transport maximum should not exceed
that capacity. Both ledger and scratch storage are preallocated. The maximum
queued input is max_frames*8, well below SDL's int clamp. `pending_frames` in the
snapshot is successful puts not yet reconciled, not a live queue query. It also
includes frames already drained by SDL since the last owner pump.

Pause first freezes the logical device, then locks the stream and reconciles its
final software-drain prefix. It retains queued input/tickets for resume. Seek or
destroy uses discard: it reconciles a trustworthy prefix, clears the remainder
without consumption credit, resets its ledger and detaches the controller pointer.
Controller destruction joins the decoder before returning. Destroy the output
only after successful controller destruction; failure leaves live storage for a
retry. SDL stream destruction closes its logical device, with no app callbacks.

A put can fail after host render reserved a ticket. That reservation cannot be
rolled back through the existing controller API. The adapter therefore faults,
attempts to pause, stops all submissions and acknowledges nothing further from
ambiguous queue state. It does not fake consumption or resume across the gap.
Owner must explicitly seek/reset the controller from its last acknowledged
position (or destroy/recreate); successful discard clears both ledgers. Pause,
lock or clear failure is not quiescence: controller seek/destroy returns false,
keeps live allocations, and can be retried. Never free on a failed destroy.

## Timing and scope limits

Owner starvation can cause SDL to exhaust its bounded queue and produce silence.
No new media is submitted while the owner stalls, and the controller clock stays
at its last reconciliation until the next pump. This is safe bounded buffering,
not continuous playback under arbitrary UI stalls. Schedule pumps frequently
enough for the chosen chunk and queue horizon; a queue of 256 frames covers only
5.33 ms at 48 kHz, while 12000 covers 250 ms. One-pump work is bounded, and a
single call deliberately does not refill the entire queue.

The clock advances in mixer-sized bursts, can lead physical audibility by an
unknown downstream tail, and can lag software mixing by an arbitrary owner stall.
No physical/backend latency bound or sample-exact visual synchronization follows.
The selected precondition is a fixed destination format for the whole generation.
Observed mismatches fail closed. A transient device-format change and restoration
between owner polls cannot be detected by those snapshots; dynamic device
migration/hotplug needs a separate reviewed event/format invalidation strategy
before physical-backend integration. The source format/maps/ratio stay private
and unchanged. Volume mixing and later backend conversion do not turn this into
a hardware clock.

This slice proves the software boundary on Linux SDL 3.4.18 dummy only. Physical
playback, macOS/Windows validation, hardware removal/recovery, video presentation,
rate control and combining real demux with this transport remain separate work.
See [PROOF.md](PROOF.md).
