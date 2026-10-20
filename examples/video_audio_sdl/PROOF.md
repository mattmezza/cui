# SDL output step 2 proof

Decision: use callback-free owner-thread pumping instead of get/postmix app
callbacks from the earlier [design checkpoint](../../docs/design/NATIVE_VIDEO_AUDIO_PLAN.md).
SDL's mixer asynchronously drains the bound stream, so owner reconciliation still
observes real software input consumption. The change removes OS/app pointer
lifetimes and callback teardown synchronization; it adds explicit owner-poll
starvation and clock-observation latency. No controller API change was required.

Inspected installed SDL 3.4.18 headers and cached release source (revision
`SDL-release-3.4.18-0-g829a65d76`). Relevant source functions:

- `src/audio/SDL_audio.c`: `SetLogicalAudioDevicePauseState` obtains the physical
  device lock via `ObtainLogicalAudioDevice`, changes logical pause state and
  releases it. The output iteration holds the device lock, skips paused logical
  devices and drains streams through `SDL_GetAudioStreamDataAdjustGain`.
  Consequently successful pause outside the stream lock synchronizes with its
  current mix iteration before final stream-locked reconciliation.
- `src/audio/SDL_audiocvt.c`: `SDL_GetAudioStreamQueued` takes the recursive stream
  mutex and reads `SDL_GetAudioQueueQueued` input bytes, capped at INT_MAX.
  `SDL_ClearAudioStream` clears that queue and resets resample offset.
  `GetAudioStreamResampleRate` bypasses resampling at equal rates/unity ratio and
  zero offset. The adapter never changes private format/ratio/maps; its fixed
  destination precondition excludes unsupported conversion histories.

These source facts establish a software queue boundary, not backend acceptance.
Tests report actual linked SDL version/revision/driver for reproducibility.

GCC 16.2.1, C11, `-Wall -Wextra -Wpedantic -Werror`, clang-format 23.1.2:
**10/10 normal and 10/10 ASan/UBSan private-wrapper CTests passed**.

| Fixture | Evidence |
| --- | --- |
| drain | Submission leaves media clock unchanged; partial drain and drain crossing ordered tickets acknowledge only the observed prefix; queue/ledger capacity enforced. |
| silence | Underrun zeros consume output without advancing media; decoded zeros advance exactly the media-frame count. |
| pause | Final prefix reconciled after freeze; remaining queue retained and consumed after resume. |
| generations | Seek reconciles valid old prefix, discards old remainder without credit and starts new tickets at requested media position. |
| failures | Injected failed put can mutate queue but receives zero credit. Clear failure preserves live controller. Explicit successful discard/seek recovers, then normal consumption resumes. |
| units | Negative, unaligned and excessive queue byte reports fault without consumption. Observed format mismatch also faults. |
| teardown | Pause/lock/clear failure prevents destruction; subsequent success detaches before output close, exactly once. |
| setup | Invalid configuration/format/nonempty queue, pause/lock/resume failures retain caller ownership or a live paused controller as appropriate. |
| unbound | Real same-format SDL stream: known float samples survive partial 24/40-byte reads exactly; input queue decreases by those exact byte counts; clear discards remainder and leaves no readable bytes. This alone is not device proof. |
| dummy | Real SDL dummy logical output asynchronously drains successful puts; owner pump observes positive media progress. Pause retains stable clock, resume progresses, seek/clear starts new generation, and deliberately starving the owner produces no invented submission or consumption. No callbacks are ever registered. |

Injected backends test error paths deterministically; only the dummy/unbound
fixtures make actual SDL claims. Dummy tests are not audible playback tests.
Ordinary host/SDL mutexes may block; sanitizer success is not a realtime proof.
