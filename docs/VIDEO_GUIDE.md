# Decoded video frames and a portable timeline

`cui/cui_video.h` supplies a decoder-neutral timeline model and a real CPU RGBA8
surface using the existing bitmap draw path. There is **no built-in codec**,
container reader, network transport, audio output, subtitle engine, recorder,
casting service or editor. The architecture roadmap remains [VIDEO_PLAN.md](VIDEO_PLAN.md).

The optional [offline audio host foundation](../examples/video_audio_host/README.md)
demonstrates bounded PCM buffering, worker cancellation, generation-safe seeks and
ordered output-consumption accounting with injected services. It is an
example-local C11 controller, not a CUI component API or device adapter. Its
deterministic tests require no network, microphone, speaker or window;
the guide includes build and invisible-display test commands. Browser video uses
the browser's existing audio decoder/output independently.

The separate [FFmpeg PCM decoder proof](../examples/video_audio_ffmpeg/README.md)
feeds that controller from real local media and converts audio to 48 kHz stereo
float. Offline tests check sample content, resampler draining, exact replay-based
seeks, a shared video/audio origin, cancellation and pending-worker shutdown.
This optional example currently admits only bounded PCM media and replays from
the beginning for seeks; it is not general compressed-media playback. Its pinned
FFmpeg package has GPLv3-or-later licensing, documented in the example. Neither
example connects to a speaker or the native video UI yet. Native sound output
therefore remains unimplemented; passing these tests does not prove audibility
or audio/video synchronization.

## Initialize and advance

```c
#include "cui/cui_video.h"

cui_video player;
bool ready = cui_video_init(&player, (cui_video_config){
    .duration = 120, .duration_known = true, .seekable = true
});
if (ready) {
    ready = cui_video_set_playing(&player, true);
    /* Once per host tick, with elapsed monotonic seconds, excluding paused time. */
    ready = ready && cui_video_advance(&player, elapsed_seconds);
}
```

The caller owns the model and UI-thread synchronization. No wall clock is read
internally. Pause leaves position unchanged; rate is any positive finite value.
Known duration may be zero; unknown duration uses `duration_known=false` and
`duration=0`. Unknown playback advances until the host calls `cui_video_finish`.
No buffering/decode state or frame queue is implied.

Seeking requires `seekable`; known duration bounds the target. Successful seeks
increment `generation`, including a seek to the same time, invalidating previously
acquired frames. Seeking exactly to the known end stops playback. Starting an
ended seekable model restarts at zero with a new generation; restarting an ended
nonseekable source fails. A model with looping enabled wraps when advancing past
its known positive duration only when seekable. Each advance that wraps increments
generation once, even if several loops elapsed. `finish` is an explicit terminal
host acknowledgement and stops regardless of loop preference.

The timeline requests no decoder action itself: the host must translate seeks,
restart and loop generation changes into source commands and acknowledge actual
results in its own controller. Do not interpret an accepted model command as a
successful asynchronous decoder seek. Keep audio-synchronized time, error,
buffering and capability handling in that host/controller until an adapter
contract is implemented.

All model commands return false without changing the model for invalid inputs,
nonfinite values, overflow, unsupported seek/restart, or exhausted generation.
Initialization sets generation to one; do not reinitialize a player while old
frames can still be submitted. Replacing a source requires retiring its frames
and outstanding draw lists before reusing generation values.

## Submit an already-decoded frame

```c
cui_video_frame frame = {
    .bitmap = {.pixels = rgba, .width = frame_width, .height = frame_height,
               .stride = row_bytes, .revision = frame_revision},
    .generation = player.generation,
    .timestamp = presentation_seconds
};
/* Inside an active frame, after the host selects the due decoded frame. */
bool submitted = cui_video_surface(ui, video_id, &player, &frame,
    (cui_video_surface_options){
        .geometry = {.width = available_width, .height = 240, .fit = CUI_IMAGE_CONTAIN},
        .opacity = 1, .description = cui_string("Video preview")
    });
```

Pixels are borrowed straight-alpha RGBA8, including any row padding. Keep them
unchanged until **every renderer has finished consuming the draw list**;
`cui_end_frame` is not a release boundary. The helper retains no lease and invokes
no release callback. A host decoder's acquired-frame lease belongs to the host;
release it exactly once after consumption or after a rejected submission that
has no outstanding users. Never recycle a decoder buffer immediately after
surface submission. A paused preview still needs its frame's pixels alive.
Increment bitmap revision when pixels change in place; use distinct retained
storage when an older draw list is outstanding.

The surface checks model generation, timestamp, bitmap stride/dimensions,
geometry, opacity and description. A timestamp is metadata: the host selects due
frames, drops late ones and schedules redraws. No automatic PTS scheduling is
provided. Contain, cover, fill, scale-down and natural-size fit reuse the existing
image layout rules and a clipped parent. Native texture import, YUV/HDR conversion,
rotation and sample-aspect metadata are unsupported. Renderer support is that of
existing CPU bitmap draws; the new tests prove software output, not every backend.

## Optional controls

The surface submits no timeline/button hit targets. The controlled
Styled composition is documented in [VIDEO_PLAYER.md](VIDEO_PLAYER.md). It emits
requests against a read-only playback view, with optional controls and explicit
host capabilities. The timeline and surface remain usable independently.

## Headless evidence

`tests/unit/test_video.c` uses a deterministic synthetic clock and two borrowed
RGBA frames. It checks pause/resume, rates, seek/end/restart, looping/generations,
unknown duration, zero duration, invalid-input atomicity and overflow. Software
rendering checks contain letterboxing, cover crop, fill, opacity, changed frame
revision and stale-frame rejection. No codec or window is needed. The test must
link `cui` and `cui_software`; the timeline and surface implementation are included
in the portable `cui` target:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build --target test_video
python scripts/test_local.py --build-dir build -R '^video$'
```

The same model and software pixel assertions run under WASI with the pinned tools:

```sh
uv run scripts/test_wasi.py --suite video
```

The generated `dist/cui_video.h` module and Go/Rust source SDKs expose the same
API. These are C APIs; language-specific video builders are not yet provided.
