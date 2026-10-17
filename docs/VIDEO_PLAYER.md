# Controlled video controls (next batch)

`cui_video_controls.h` adds optional Styled controls above the portable video
surface. This extension is independent of the frozen78-component baseline.
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

The slider uses normalized0..1 Core numeric semantics because that ABI is float;
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
when those features exist. This first bar has no auto-hide timer or overflow menu. Buttons use44-unit touch
targets and flow into additional rows within the selected width (at least160
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
