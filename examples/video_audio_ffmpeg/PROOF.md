# Decoder step 1 evidence

Implemented only the file decoder/resampler service described in
[the native audio plan](../../docs/design/NATIVE_VIDEO_AUDIO_PLAN.md).
This is a foundation for native audio, not implemented native sound support.

GCC 16.2.1, portable C11, `-Wall -Wextra -Wpedantic -Werror`, pinned clang-format
23.1.2: standalone normal and ASan/UBSan builds, 9/9 private-wrapper CTests pass.
No permission, physical audio output, network or desktop session was used.

| Check | Actual evidence |
| --- | --- |
| PCM | Generated 10-frame FFV1 video muxed with 24000 stereo S16 frames; all normalized output samples compared against fixture reference; known signed impulses checked. |
| Resample/drain | 22050 input frames at 44.1 kHz become exactly 24000 output frames at 48 kHz; every sample compared with pinned FFmpeg CLI SWR reference at absolute tolerance 0.000002. Last impulse is 64 source frames before EOF, so tail/drain contributes to comparison. |
| Seek | Full output after frame 7301 matches the unsought reference suffix; seek beyond EOF returns zero frames. Replay preserves conversion phase for 44.1 kHz fixture. |
| Shared origin | Video starts at zero, audio at 125 ms. Output includes exactly 6000 timeline zeros and the complete 24000-frame audio; seeking retains the shared timeline. |
| Bounds/errors | Packet-byte, converted-spill, packet-count and timeline caps trigger LIMIT. Malformed media returns IO. FLAC rejected as unsupported. Excessive initial offset and discontinuous timestamps rejected. |
| Generation | Sticky old cancellation rejects the old seek; fresh generations decode and replay normally. 50 actual controller worker seek/pause/play cycles retain exact consumed-frame positions. A full 512-frame ring stops further conversion until consumption; full EOF consumption ends at exactly frame 24000. |
| Late return/shutdown | A test-only condition gate holds an actual completed FFmpeg read before return. Seek interrupts/releases it: old PCM is discarded and new PCM starts at the requested frame. Destroy also interrupts/releases it and joins before service storage is freed. |

The condition gate proves the injected interface/controller lifetime boundary;
it is not a claim that FFmpeg/OS disk I/O always returns within a deadline.
The tests use an injected no-device output whose explicit consumption reports
exercise only controller accounting. They prove neither SDL mixer consumption
nor physical audibility. Normal tests compare against the same pinned SWR
implementation via a separate CLI path, plus independent known impulse/count
checks; they do not establish a different resampler's numerical equivalence.

Deferred: SDL software-mixer consumption adapter and pause/flush evidence, actual
video presentation against that clock, compressed codecs/priming, arbitrary seeks,
negative/unknown origins, timestamp discontinuity recovery, mono and other PCM
representation fixtures, long files, hardware output and bounded physical tail.
No controller contract change or Core/public framework API change was needed.
Root integration owns canonical build/CI, metadata and consumer-skill updates.
