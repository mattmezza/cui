# Video architecture proposal

Audit baseline: `b96c6844`. No decoder dependency has been selected or installed.
This document proposes a reusable playback surface, with controls composed above
it, rather than putting decoding, transport and UI in one component.

The portable timeline and borrowed CPU RGBA surface are now available in
`cui/cui_video.h`; [the implementation guide](VIDEO_GUIDE.md) describes their
actual behavior and tests. Decoder integration, audio, frame leases/queues,
automatic scheduling and the full controls component below remain planned.

## Existing foundation and its limit

`include/cui/cui_media.h` already defines borrowed image frames (RGBA-sized
stride validation, revision, duration), an image loader, contain/cover/fill layout,
and image animation state with caller-provided time and next-wake scheduling.
`src/base/cui_media.c` validates frames and polls a host-side image cache. That
is useful precedent for ownership and frame presentation; it is not a streaming
video decoder, audio clock, seek engine, recorder or editor.

`cui_bitmap` in `include/cui/cui.h` is borrowed straight-alpha RGBA8 with stride
and revision. It remains valid through renderer completion. A first video surface
can reuse `cui_paint_bitmap` and image-fit geometry with decoded RGBA frames. It
must not preload an entire movie into the animated-image frame array (currently
bounded at 4096 entries). Dynamic streaming requires bounded queues and explicit
frame release, buffering and end-of-stream behavior.

## Portable layers

1. **Decoded-frame surface:** accept frame identity/generation, dimensions,
   RGBA8 stride, pixel data and presentation timestamp. Retain/release a frame
   lease until the renderer finishes consuming it; never let the decoder recycle
   pixels at submission time. Draw using existing fit/crop rules. Start with CPU
   frames; zero-copy platform textures require separate optional adapters.
2. **Playback state/controller:** stopped/loading/playing/paused/buffering/ended/
   failed states; position, known/unknown duration, seekability, playback rate,
   mute/volume and error/capability values. Commands request state changes and
   asynchronous results acknowledge them. No codec handles or OS types in Core.
3. **Clock and scheduler:** host monotonic time for silent playback; optional
   audio master clock when audio is enabled. Compare timestamps, present due
   frames, drop late frames by documented policy and request the next wake.
   Pause/resume must rebase time; seek uses a new generation so stale frames and
   callbacks cannot reappear. Avoid busy polling and unbounded decode queues.
4. **Composed controls:** buttons, timeline slider, time display, loading/error
   overlay, volume, captions and fullscreen request. Keep these optional so the
   same surface supports thumbnails, backgrounds and applications with custom
   controls. Keyboard focus, accessible names/status, seek increments and reduced
   motion/autoplay policy belong in the UI/host contract.
5. **Optional adapters:** decoder/container/network input, audio output/clock,
   subtitle sources, platform fullscreen and GPU frame import. Adapter capability
   negotiation must expose unsupported seek/rate/audio/captions. Worker threads
   may decode; UI callbacks and font/UI state stay on their documented thread.

A proposed decoder-neutral interface should offer open/poll/command/close with
bounded events and explicit frame acquisition/release. Do not finalize ABI before
agreeing on time units, rational timestamp conversion, thread/cancellation rules,
seek completion, queue bounds, frame orientation, sample aspect ratio, color
metadata and source lifetime. Begin with RGBA8 SDR; document conversion/color
limitations. YUV planes, HDR, GPU surfaces and hardware decoding are later
capabilities, not implied by a CPU bitmap prototype.

## Different product scopes

| Scope | Required pipeline | Relationship to playback |
| --- | --- | --- |
| Local/streamed playback | Demux/decode, timestamp scheduling, optional audio output | Initial target; controls can be composed independently. |
| Recording | Capture source, timestamped audio/video, encoder, muxer, file lifecycle | Separate adapter pipeline; recording a CUI surface needs a defined readback boundary. |
| Casting | Device discovery/session/transport, remote control and remote playback state | Separate host/service integration with latency and permission requirements. |
| Editing | Source model, timeline, trims/tracks, preview, export/render/encode | Separate application model that may reuse the playback surface. |
| Screen/camera capture | Platform capture APIs, permissions, device lifecycle | Optional input adapter; not included in a decoded-frame widget. |

## Deliverable sequence and tests

First implement a renderer-independent playback state machine with a fake clock
and synthetic decoder frames. Prove pause/resume, buffering, cancellation,
end-of-stream, unknown duration, seek generations, rate changes and bounded
queues deterministically. Add a CPU RGBA surface and fit/clipping/opacity/revision
checks using software output; verify SDL under invisible Xvfb and Canvas headless.
Assert that frames remain leased until consumption and release exactly once.

Then compose accessible controls against the fake controller. Only after the
interfaces work should a separate decision compare decoder adapters against
supported platforms, license/distribution constraints, pinned versions, codec
availability, audio synchronization and package size. No dependency is necessary
to review or test the first two deliverables. A real local-file adapter must prove
play/pause/seek/audio synchronization and cleanup before claiming video support.
Recording, casting and editing each need separate acceptance criteria and scope.
