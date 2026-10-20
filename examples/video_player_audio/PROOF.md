# Native sound example proof

GCC16.2.1, C11, `-Wall -Wextra -Wpedantic -Werror`, pinned clang-format23.1.2:
**22/22 normal and22/22 ASan/UBSan focused wrapper checks passed**, including real
native rendering on private Xvfb with SDL dummy audio. Runtime SDL revision:
`SDL-release-3.4.18-0-g829a65d76`. No desktop, speakers, physical output, network,
permission prompt or installation was used.

| Evidence | Result |
| --- | --- |
| Combined production pipeline | Actual muxed20 FFV1 red/blue frames and96000 stereo PCM frames, actual video worker + actual FFmpeg/SWR audio worker + actual asynchronous SDL dummy output. Every acquired playing video frame is due at observed mixer media time; terminal media clock is exactly96000. Repeat with44.1k PCM input and with replay. |
| Pause/seek/preview | Pause holds position and leased pixel revision through40 owner polls. Seeking1.25s holds exact audio target60000 while displaying later paused video preview; playing hides that future preview until due. Seek to exact EOF completes without fabricated media. Borrowed pixels survive a seek in the same composition before next poll. |
| Common video origin | Actual muxed fixture has video start125ms and audio/container start0. Common-clock video mode preserves125ms PTS; metadata carries common origin and worker generation/end frame. Legacy open behavior remains unchanged. |
| Partial failure and ownership | Injected video rejection after actual audio seek yields failed/paused session. Injected first audio-destroy failure retains live session for retry; second actual destroy joins and frees in order. Missing media reports failure; unequal1s audio/2s video endpoints fail at real EOF. |
| Device invalidation | Actual SDL event-watch path handles synthetic output added/removed/format change as sticky fault without media credit. Clear/seek cannot restore validity; capture notifications ignored.100 concurrent actual SDL dispatch/open/remove cycles run under ASan. These are not physical hotplug tests. |
| Transport regression | All10 original injected/unbound/dummy output cases pass against the guarded backend: ordered partial drain, underrun versus decoded zeros, pause/retention, generation discard, failed put/clear recovery, invalid byte counts, setup, quiescence retry and dummy starvation. |
| Legacy video regression | Existing unchanged `examples/video_player/test_decoder.c` compiles/runs against additive video adapter, including actual RGBA/software pixels, pause lease, seek, stale release and missing-file cleanup. |
| Native consumer UI | Actual callback player on private Xvfb/software renderer with forced SDL dummy; rendered red/pause/blue BMP interior pixels verified. It observes mixer EOF96000 and retains committed seek60000 rather than adopting preview1.3s. No placeholder video or synthetic PCM provider replaces failed decode. |

The common-clock video factory is additive/example-private. The default factory
still subtracts video stream origin. Both use the same existing3-slot lease
engine. Core/public framework APIs and backend window/fullscreen APIs are unchanged.
The sound session admits known zero container origin; common-mode metadata does
not claim general edit-list/nonzero-origin consumer support. The endpoint
comparison uses a fixed100ms tolerance for this initial profile, not arbitrary
frame cadence matching. Audio/video endpoint compatibility is validated at actual
EOF rather than guessed from container duration alone.

SDL source inspected for invalidation lifetime: cached3.4.18
`src/events/SDL_eventwatch.c` dispatch holds the watcher mutex across callbacks;
owner removal uses the same mutex, outside all app event callbacks and stream/
device locks. Backend endpoint callbacks perform atomic store only. The UI smoke
caught real deferred startup enumeration; normal first owner poll now precedes
guarded session creation, without flag reset or nested pumping.

The headless probes and native smoke prove software route/accounting/rendering;
they do not prove acoustically audible output, hardware/DAC timing, actual
hotplug recovery, arbitrary UI-stall continuity, macOS/Windows builds, HDR/color
metadata, MP4/AAC/Opus, compressed priming or long/fast seeking. Fullscreen remains
the documented preexisting native omission; rate capability is deliberately
absent. No broad component parity promotion follows.

Root integration owns canonical CI/build registration, metadata, guides and
consumer-skill regeneration. Local builds under home cache requested only the
new example/tests and their CUI/text/software dependencies, not all root targets.

Startup failure regression: the actual callback executable rejects a protocol-like
local path synchronously before session creation, prints zero frame/generation
without dereferencing a missing session, and exits with status1. Missing-font and
unavailable-video-driver cases cover failed initialization before acquisition and
after font acquisition. All three run only under the private wrapper, with dummy
audio; no output-device activation is required. Sanitizers report no errors.
The runner publishes callback state before calling initialize and routes quit to
the application even when initialize fails. Application-owned partial resources
are therefore unwound from its fields, independently of output host/UI pointers
(which are published only on success). Context, SDL host and font destructors
accept NULL; audio subsystem shutdown now pairs only successful audio initialization.
