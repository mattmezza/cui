# Controlled video controls

`cui_video_controls.h` adds optional Styled controls above the portable video
surface. This extension is independent of the frozen 78-component baseline.
It does not decode, play audio, change native fullscreen, or acknowledge commands.

```c
#include "cui/cui_video_controls.h"

/* Retain one zero-initialized state per stable controls ID. */
cui_video_controls_options options = cui_video_controls_defaults();
options.on_request = request_playback;
options.user = application;
options.width = 320;
options.visible |= CUI_VIDEO_CONTROL_FULLSCREEN;
cui_video_controls(ui, controls_id, theme, &application->controls,
                   application->confirmed_view, options);
```

The view is copied by value. Its timeline is confirmed application state. Each
request contains `source_id`, observed timeline `generation`, and a monotonically
increasing `sequence`. The callback may accept or reject it without the controls
writing playback state. It must not destroy the context or controls state during
submission. A missing callback makes controls read-only. Sequence exhaustion
makes them read-only; retire outstanding work before replacing their state.
Source IDs must be nonzero and must not be reused while old frames or requests
can survive. Source or generation changes clear local scrub presentation.

`PLAYING` and `FULLSCREEN` carry the desired boolean; `SKIP` carries a signed
seconds delta; `RATE` carries the explicitly configured `requested_rate`.
`SEEK` carries absolute double-precision seconds and BEGIN/UPDATE/COMMIT/CANCEL.
BEGIN starts preview, UPDATE adjusts it, COMMIT requests the final decoder seek,
and CANCEL restores the gesture's starting preview. An application can implement
live seeking, but should bound outstanding seeks and ignore obsolete completions.
Keyboard changes emit all three accepted phases per key. Gesture preview is not
a second authoritative playback timeline. Removing a timeline/source invalidates
its local preview; it does not issue a stale cancellation request.

The slider uses normalized 0..1 Core numeric semantics because that ABI is float;
the accessible description gives position/duration in seconds. It retains double
seconds in requests. Space/Enter use existing Button behavior; focused timeline
arrows use `seek_step`, Home/End use bounds. Shortcuts never inspect global keys.
Buttons retain accessible labels when icon-only. Set labels to localize them.
Use each `*_icon` catalog name/path and `theme.icon_loader` for replacement SVGs.
No raw SVG parsing or codec types are added to the public player contract.

Visibility and capability are separate: absent `visible` flags submit no hit
regions or semantics; visible unsupported actions are disabled. Seek additionally
requires a known positive duration and a seekable timeline. Loading/buffering/error
views disable playback/seek/rate; fullscreen remains a separate host capability.
Fullscreen display follows only the confirmed `view.fullscreen` value.
Volume/mute/captions have no invented state; supply separate host-backed controls
when those features exist. This first bar has no auto-hide timer or overflow menu.
Buttons use 44-unit touch targets and flow into additional rows within the selected width (at least 160
logical units).

`cui_video_player` composes surface and optional controls. A null frame reserves
surface geometry for caller-supplied loading/error content. Its frame wrapper adds
source identity to the existing generation check. Mismatches fail before UI
submission. Pixels remain borrowed until every renderer has consumed the draw
list. No release callback or implicit decoder lease is provided.

## Extended Base slider

`cui_base_slider_controlled` is an additive unstyled double-precision API.
Legacy `cui_base_slider` retains its float pointer mutation and return semantics.
The new function accepts confirmed value by value, caller-owned `cui_slider_state`,
and `cui_slider_options`. It emits typed synchronous requests with copied source
events. Invalid arguments return false without changing state. Disabled/read-only
transitions cancel an active drag; no callback means no requests. State and user
callbacks must survive the call. Submit it inside a box with its own stable ID.

## Real optional decoder example

The [silent FFmpeg example](../examples/video_player/README.md) decodes on a worker,
retains bounded frame leases and demonstrates confirmed play/pause/seek/rate
requests. Its strict external harness runs portable controls, software decoded
pixels and private-Xvfb native decoded captures. It supplies no built-in codec or
universal format/browser/audio support; see its exact evidence and version/license
recording before using it as a host-adapter starting point.

## Browser gallery extension

The gallery's **Video** entry is an extension outside the frozen 78-family GPUI
baseline. Build and exercise it locally (Chromium runs headless):

```sh
python scripts/build_web.py --output /tmp/cui-video-web
uv run tests/web/test_video_gallery.py --offline
uv run tests/web/test_video_gallery.py --directory /tmp/cui-video-web
python -m http.server --directory /tmp/cui-video-web
# Open http://localhost:8000/#Video
```

The browser decodes a checked-in original, silent 12-second 320×180 sample using
`HTMLVideoElement`. The visible surface, buttons and timeline are CUI draw lists;
there are no HTML media controls. Play/pause, two-second skips, keyboard timeline
seeks and 1× / 1.5× rate requests are acknowledged from actual media state. The
sample is muted and loops. Fullscreen is a browser capability and may reject a
request; host errors are displayed explicitly. This is not audio playback or
universal codec/browser support, and Safari has not been validated.

The host chooses VP8 WebM or H.264 MP4 using `canPlayType`, with the alternate
encoding available if decoding fails. It fetches at most 512 KiB into a Blob so
this small sample can seek even when static hosting does not implement byte
ranges. Fetches are aborted and Blob URLs revoked on route exit. Larger streaming
applications need a separate buffering/range policy. Rebuild the original assets
with `python web/assets/video/generate.py`; its receipt records local FFmpeg
version, exact commands and output hashes. The generator requires local `libvpx`
and `libx264` encoders and uses bit-exact output settings; two consecutive local
runs produce identical files. FFmpeg is a fixture-generation tool,
not a browser runtime dependency; no media is downloaded during generation.

Decoded frames are copied into one fixed 230,400-byte WASM buffer. Session and
timeline generation checks reject stale submission. A separate decode ticket
retires pending frame callbacks during every seek; the timeline generation changes
on commit/cancel/skip so active scrubbing survives updates. Seek cancel restores
the position captured at begin; browser decoder completion remains asynchronous.
Pixels stay unchanged throughout synchronous Canvas consumption, and revisions
advance on each accepted frame. Host requests are copied into a 32-entry queue
and drained after rendering, never through CUI reentry. A batch exceeding 32 requests
is rejected in full with a visible limit message, resetting scrub presentation
and restoring an already-active scrub origin. This sample does not promise
unbounded gesture batching. Late operation promises are fenced by session,
operation sequence and decode ticket. No frame callback loop runs while paused;
leaving the route cancels scheduling, unloads media and exits only fullscreen
owned by this preview.

The focused Chromium test decodes both the actual WebM and MP4 assets, including
alternate-source recovery after a deliberately invalid preferred encoding. It
separates real encoded decoding/CUI interaction from
injected host-error protocol checks. It checks rendered pixel changes, confirmed
play/pause/seek/rate, keyboard routing, mobile DPR2 geometry, stale frame rejection,
route cleanup and the separate extension catalog. Other browser/platform codec
and fullscreen combinations remain unverified.
