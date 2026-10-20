# Native decoded video with SDL software-mixer audio: integration checkpoint

Status: design/probe only, based on `95527cf2` (audio service `ef93fec5`,
controller foundation, SDL transport). No production/example integration changed.
The next deliverable should be a runnable native video **with sound** example
for an explicit supported local-file profile, rather than more disconnected
foundation APIs. Hardware audibility remains a manual validation claim.

## What exists, and the actual gaps

| Existing source | Actual behavior | Required integration decision |
| --- | --- | --- |
| `examples/video_player/main.c:requested/iterate` | Controlled requests, owner-thread state, SDL monotonic wall-time advance, 3-slot decoded frame leases; paused seek preview replaces model position with acquired video timestamp. | Audio mode must use observed consumed media frames exclusively; wall time and preview timestamp must not move audio position. |
| `examples/video_player/decoder.c:decode` | Local-file FFmpeg video worker, FFV1 fixture, bounded RGBA slots, seek/flush/stale-publication guards. Video PTS subtracts **video stream start_time**; AVIO interrupt currently observes shutdown, not seek. | Add an example-private common-container-origin mode/metadata and cancellation audit; unchanged stream-relative origin is insufficient for arbitrary A/V offsets. |
| `examples/video_audio_ffmpeg` | PCM-only, common container origin, fixed stereo48k output, SWR drain, bounded linear replay/trim seek, <=10s timeline. | Admit a usable muxed PCM fixture; reject unsupported input explicitly. Do not advertise the browser MP4/AAC or WebM/Opus sample as supported native sound. |
| `examples/video_audio_sdl` | Callback-free owner pump, exact successful-put minus queued-input prefix, fixed both-end F32/stereo48k/ratio1. | Schedule pump during all active playback, preserve fault/reset semantics and add sticky device-change invalidation. |
| `cui_sdl3_run` / `cui_sdl3_size` | Callback-main runner protects reentry and supports live native modal resize; UI idle switches callback hint to waitevent. | Keep runner and invalidate while playing/loading/seeking so sound is not dependent on user input. No nested event pump or blocking decode in frame callback. |
| `cui_window_host` / SDL3 backend | Window query includes fullscreen; action enum has no fullscreen action, SDL handle is private. Existing native video explicitly omits fullscreen. | This is an existing native gap, not behavior available to preserve. Add a narrowly coordinated SDL host fullscreen action before claiming fullscreen support; keep browser fullscreen unchanged. |

Core/Base/components remain codec/platform free. No public framework schema or
playback state machine is required for audio. A narrowly typed optional native
host function for fullscreen, if approved, is adapter work and must be reviewed
separately from this example's private session API.

## Supported first consumer example

Prefer a new optional `examples/video_player_audio` using the existing callback
runner and video lease adapter, with an independently testable private session
controller. Retain the existing silent example/explicit silent mode for video-only
tracks. Audio-open/decode/output failure must be a visible failure in audio mode;
never silently claim successful sound by switching to silent playback.

Initial documented input profile: immutable regular local Matroska, FFV1 SDR video
and stereo S16LE PCM audio at48k or44.1k, known nonnegative common container origin,
fixed layouts/rates, duration <=10s, bounded raster/packets/frames as the existing
adapters. Generate a 2-second 64x32 red/blue clip plus signed PCM impulses (or an
explicit manual-listening tone fixture) locally. The same consumer command can
play that actual muxed file with SDL's real default output **only when a human
explicitly runs it**; automation forcibly selects dummy. Include an exact FFmpeg
command for converting a user's short local clip to the supported profile, with
license/size/quality tradeoffs and no automatic conversion/download.

This is useful native video with sound for that profile. It is not a general MP4
player, and must say so in title/status/readme. AAC/Opus priming, compressed seek,
long files, arbitrary metadata/edit lists, playback rates and codecs stay a scoped
followup; current browser Opus/AAC sound evidence does not establish native support.
Rate capability/control must be absent in audio mode (rate fixed1). Audio is
unmuted by default in the explicit interactive sound example, not hidden behind a
fake volume control unsupported by the current transport.

## Session timeline and presentation

Use one immutable source ID and one owner session generation. Keep an explicit
mapping to the audio controller's generation (it starts at1 and increments on
successful seek) and the video generation; compare both before publishing state.
Do not assume unrelated counters remain aligned after a partial failure/reopen.

Require video and audio normalized timestamps from the same rational container
origin. Proposed compatible video-adapter extension: an example-private open mode
that validates known origin and computes each PTS with `av_rescale_q` into48k
integer ticks, subtracting the same normalized container origin used by audio.
Expose copied origin/clock metadata for an owner compatibility check. Preserve
existing stream-relative open behavior for the silent example. Reuse the same
3-slot lease engine rather than copy its decoder implementation. Admission must
reject unknown, incompatible or negative normalized timestamps; no guessed
independent stream-zero reset. An alternative smaller first step can explicitly
reject any video/container origin mismatch, but cannot be called general A/V
origin support. The initial probe below uses equal zero origins deliberately.

Each owner iteration: process host input, validate session/format state, pump SDL,
copy audio snapshot, then assign confirmed position=`position_frames/48000.0` for
the mapped generation. Never add `cui_sdl3_frame_delta` to media time. Acquire the
latest due video lease at that position, dropping older queued frames; keep the
previous lease until synchronous renderer consumption completes, then release
exactly once. Retain the last valid frame during underrun. A late video decoder
may drop frames without advancing audio; a late audio decoder produces tagged
underrun padding and stalls media time. No worker calls CUI, SDL event pumping,
window actions or renderer APIs.

The audio clock is mixer-input consumption. It advances in mix bursts and may
lead physical sound by unknown downstream latency. Due-frame selection proves
synchronization against that observable boundary, not lip-sync at speakers. Report
that boundary in example status/documentation; do not invent a DAC correction.

Paused seek has a separate presentation preview. Quantize/clamp the committed
target to an integer48k frame, quiesce/discard output, then seek audio and video.
The earliest video frame at/after target may be displayed while paused, but its
frame PTS must **not replace** the confirmed audio target. Once playing, do not
present that future preview as due until the audio clock reaches its PTS; retain
an explicitly labeled preview/blank until then. The current silent example's
preview acknowledgement is unsuitable for this mode.

EOF policy for the first profile: require matching known audio/video timeline
endpoints within one video-frame rounding interval and reject large mismatches
at admission. Actual audio/controller EOF plus drained media controls finish;
container duration alone must not finish before the last audio sample. Continue
acquiring video through the terminal audio time, retain the final frame, stop
feeding redundant padding, and show ended only after current media drains. A
video tail beyond audio needs a later explicit clock policy, not accidental wall
clock fallback. Unknown duration and unequal-track-end recovery are deferred.

## Owner transactions, faults and resource lifetime

Play succeeds only after video metadata/origin validation and audio prefill; set
confirmed playing only after audio output start succeeds. Pause calls audio
pause first (freeze/reconcile), copies final consumed position, then confirms
paused UI state. Failure leaves session failed/paused with no fictitious success.
Audio mode does not make automatic worker waits in a UI callback: readiness is
polled and loading/seeking schedules another owner frame.

Seek computes the next session state, pauses/quiesces audio, discards queued old
output and requests both workers. Confirm the new generation/target only after
both requests are accepted; asynchronous readiness remains loading/seeking. If
one side accepts and the other fails, never roll back counters or resume the old
session: keep output quiescent, report error and require explicit whole-session
reopen/reset. Old leased pixels stay valid until released, but cannot enter the
new generation. Play during pending seek is a requested intent applied after
both new-generation workers are ready, not permission to mix stale PCM.

On a failed SDL put, no additional acknowledgement is possible across its
unsubmitted ticket. Pause/discard and reset from the last acknowledged source
frame, or destroy/recreate the whole session. Record error before cleanup; do not
hide partial failure behind a new healthy UI generation. A failed stop/clear is
not release authority. Preserve live host/output/service and retry cleanup; never
free service userdata before successful controller join. Source-read I/O can
still delay joining; no hard shutdown deadline is claimed.

Shutdown ordering: stop owner pumps/input, pause/quiesce/discard audio, successfully
destroy/join audio controller, destroy its FFmpeg service, remove/quiesce device
watch and destroy SDL output, release video lease after last renderer use, close/
join video worker, then destroy Core/window/font. A failure at an ownership
boundary preserves that resource; do not continue freeing objects it can reach.
Initial-open failure must unwind only resources actually acquired.

## Device-format invalidation policy

Observed format mismatch currently faults before consumption credit, but a
format change and restoration between owner polls is not observable from format
snapshots. For real output integration, add a private SDL event watch whose only
userdata is a dedicated atomic invalidation endpoint (no host/UI/sink pointer).
After SDL audio init and paused stream open, register the watch before validating
format/starting the generation. Any output-device ADDED, REMOVED or FORMAT_CHANGED
notification conservatively sets a sticky invalid flag, including notifications
for other output devices. No filtering/remapping or silent default migration is
assumed. The backend `format` check must include this flag under the same stream
transaction before queue reconciliation. Never clear it on seek or restore it
merely because the current format matches again: require explicit whole output/
controller recreation and fresh validation. A late startup notification may fail
closed and require retry. Ensure those SDL event types remain enabled and no
application event filter suppresses them.

Pinned3.4.18 source evidence: `src/events/SDL_eventwatch.c` holds its watcher-list
mutex across callback dispatch and uses the same mutex for removal. Remove from
the owner outside every callback and outside stream/device locks; it waits for
in-flight dispatch before endpoint free. Callback performs only atomic store,
never CUI, allocation, output pause, locks, wake userdata or SDL lifecycle calls.
This avoids an unbounded retired-endpoint pool on the inspected build. Do not
infer generic cross-version quiescence from the API name; rerun concurrent actual
watch/removal tests on every admitted SDL build. OS notification guarantees and
physical hotplug behavior still need platform validation; synthetic SDL events
prove sticky handling/lifetime only, not real hardware notifications. Do not
advertise universal dynamic-device recovery from those fixtures.

## Resize and fullscreen

Keep `cui_sdl3_run`, layout from `cui_sdl3_size`, ordinary responsive controls and
borrowed frame leases. Modal resize can delay owner pumping and exhaust the
bounded audio queue; SDL then emits silence and media time stops after queued
media drains. Owner resumes/reconciles safely. Continuous sound throughout an
arbitrary modal stall is not promised by callback-free pumping. No worker or
event watch attempts to repaint or pump the UI during the stall.

Fullscreen is missing from the audited native example. Preferred next action:
coordinate a minimal optional SDL host `set_fullscreen` operation with observed
window-state confirmation (plain bool/types, no SDL handle in public types), and
wire the existing capability-gated video request to it. Native example needs no
Core fullscreen schema; `cui_video_view.fullscreen` already exists. Only set the
confirmed view after observed success; keep resize/aspect/lease logic unchanged.
A private example using `SDL_GetWindows` to guess the backend's window is rejected
as ambiguous ownership. If the integrator defers that host action, explicitly
retain the current native omission and do not claim fullscreen preservation/
implementation. This decision must be settled before final consumer-example scope.

## Reviewable implementation slices and evidence

1. New private session controller + combined no-window fixture. Reuse production
   audio decoder/output and actual video worker; add common-origin video mode and
   sticky device invalidation in separately coordinated existing adapter files.
   Assert actual PCM/sample counts, colored decoded pixels selected against
   observed mixer clock, pause retention, seek target versus preview, two-track
   EOF, stale generation, injected one-sided seek failure and shutdown ownership.
2. New runnable optional sound player using existing UI controls/runner, supported
   muxed fixture generation, explicit help/errors, rate omitted. Implement the
   approved fullscreen action or document the existing omission. External CMake
   harness builds needed targets only; preserve exact SDL/FFmpeg pins and license
   audit, adding no codec dependencies to Core.
3. Private-Xvfb native smoke with SDL dummy audio: presented red/blue captures,
   pause/seek and generation, resize, fullscreen if available, mixer-frame counts,
   no desktop/physical audio. Continue independent headless non-window tests and
   strict GCC/Clang ASan/UBSan CI. Manual real default-device launch/listening is
   separate evidence, never an automated side effect.

Root owns canonical build/CI, metadata and consumer skill guidance/regeneration.
Production work waits for review and explicit ownership of existing video/native
host files. No browser regressions or manifest parity promotion follows this plan.

## Actual combined feasibility result

An external no-window harness compiled **unchanged** production video decoder,
audio controller, FFmpeg audio service and SDL output from this checkpoint.
Strict GCC16 C11/Werror and ASan/UBSan each passed2/2 wrapper CTests (local mux
fixture plus combined session). Actual SDL runtime:
`SDL-release-3.4.18-0-g829a65d76`, driver=dummy. Exact libraries: avformat/avcodec
63.1.102, avutil61.1.102, swscale10.1.102, swresample7.1.102; CLI n9.0.2.
No Core context, window, renderer, actual device or network was opened.

The harness muxes20 actual FFV1 red/blue frames with96000 stereo S16 PCM frames,
asserts actual decoded RGBA colors for every acquired due lease, observes media
advance only after real asynchronous dummy draining, freezes position/lease on
pause, coordinates seek57600/1.2s across both workers, verifies exact terminal
media frame96000 and both decoder EOFs, then releases/joins in ownership order.
It does not prove nonzero video-origin handling, unequal end policy, presentation
pixels, device invalidation, physical sound or the proposed fullscreen action.

Retained reproducible harness and builds:
`/home/matteo/.cache/cui-validation/video-audio-hookup-probe/{probe.c,fixture.py,CMakeLists.txt,build,build-san}`.
Run with dedicated home-cache TMPDIR through `scripts/test_local.py --build-dir
.../build -- -V` (or `build-san`). Complete probe source follows so this design
checkpoint is reproducible without relying on retained cache files. Extract the
three blocks into an external directory; replace CMake ROOT with the checkout
path. Configure normal and sanitizer builds (`-DCMAKE_C_FLAGS=-fsanitize=address,undefined`),
build, then run each through the private wrapper. This appendix is a design
experiment, not a new installed production example.

### Probe `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(probe C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(ROOT /home/matteo/.cache/cui-worktrees/video-audio-integration)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET sdl3=3.4.18 libavformat=63.1.102 libavcodec=63.1.102 libavutil=61.1.102 libswscale=10.1.102 libswresample=7.1.102)
add_executable(probe probe.c ${ROOT}/examples/video_player/decoder.c ${ROOT}/examples/video_audio_host/host.c ${ROOT}/examples/video_audio_ffmpeg/decoder.c ${ROOT}/examples/video_audio_sdl/output.c ${ROOT}/examples/video_audio_sdl/sdl.c)
target_include_directories(probe PRIVATE ${ROOT} ${ROOT}/include)
target_link_libraries(probe PRIVATE PkgConfig::DEPS Threads::Threads m)
target_compile_options(probe PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
enable_testing()
add_test(NAME fixture COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/fixture.py ${CMAKE_CURRENT_BINARY_DIR}/media)
set_tests_properties(fixture PROPERTIES FIXTURES_SETUP media)
add_test(NAME combined COMMAND probe ${CMAKE_CURRENT_BINARY_DIR}/media/fixture.mkv)
set_tests_properties(combined PROPERTIES FIXTURES_REQUIRED media TIMEOUT 20 ENVIRONMENT SDL_AUDIODRIVER=dummy)
```

### Probe `fixture.py`

```python
import pathlib,struct,subprocess,sys
p=pathlib.Path(sys.argv[1]);p.mkdir(parents=True,exist_ok=True)
(p/'video.raw').write_bytes(bytes((255,0,0,255))*64*32*10+bytes((0,0,255,255))*64*32*10)
(p/'audio.raw').write_bytes(b''.join(struct.pack('<hh',12000 if i%4800==0 else 0,-8000 if i%4800==0 else 0) for i in range(96000)))
subprocess.run(['ffmpeg','-nostdin','-v','error','-y','-f','rawvideo','-pixel_format','rgba','-video_size','64x32','-framerate','10','-i',str(p/'video.raw'),'-f','s16le','-ar','48000','-ac','2','-i',str(p/'audio.raw'),'-c:v','ffv1','-pix_fmt','bgr0','-c:a','pcm_s16le',str(p/'fixture.mkv')],check=True)
```

### Probe `probe.c`

```c
#include "examples/video_audio_ffmpeg/decoder.h"
#include "examples/video_audio_sdl/output.h"
#include "examples/video_player/decoder.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <threads.h>
static void delay(void) {
  struct timespec t = {0, 1000000};
  thrd_sleep(&t, NULL);
}
static void prefill(video_audio_host *h) {
  for (int i = 0; i < 10000; i++) {
    video_audio_snapshot s = video_audio_poll(h);
    assert(!s.failed);
    if (s.pcm_frames >= 1024)
      return;
    delay();
  }
  assert(0);
}
static void frame(video_decoder *v, uint64_t g, double due, video_lease *held,
                  bool *have) {
  video_lease next = {0};
  assert(!video_decoder_poll(v).failed);
  if (video_decoder_acquire(v, g, due, &next)) {
    assert(next.frame.frame.generation == g);
    assert(next.frame.frame.timestamp <= due + 0.000001);
    const unsigned char *p = next.frame.frame.bitmap.pixels;
    assert(next.frame.frame.timestamp < 1 ? p[0] > 240 && p[2] < 10
                                          : p[2] > 240 && p[0] < 10);
    if (*have)
      assert(video_decoder_release(v, held));
    *held = next;
    *have = true;
  }
}
int main(int argc, char **argv) {
  assert(argc == 2);
  assert(SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) == 0);
  assert(SDL_Init(SDL_INIT_AUDIO));
  video_decoder *v = video_decoder_open(argv[1], 1, 1);
  assert(v);
  video_audio_ffmpeg *a =
      video_ffmpeg_create(argv[1], (video_ffmpeg_limits){0});
  assert(a);
  video_sdl_output *o = video_sdl_open(4800, 1024);
  assert(o);
  video_audio_host *h = video_audio_create((video_audio_config){
      4800, 512, video_ffmpeg_interface(a), video_sdl_interface(o)});
  assert(h);
  video_lease held = {0};
  bool have = false;
  prefill(h);
  assert(video_audio_play(h));
  for (int i = 0; i < 10000 && video_audio_poll(h).position_frames < 12000;
       i++) {
    assert(video_sdl_pump(o));
    frame(v, 1, video_audio_poll(h).position_frames / 48000.0, &held, &have);
    delay();
  }
  assert(have);
  assert(video_audio_poll(h).position_frames >= 12000);
  assert(video_audio_pause(h));
  uint64_t paused = video_audio_poll(h).position_frames,
           revision = held.frame.frame.bitmap.revision;
  for (int i = 0; i < 40; i++) {
    assert(video_sdl_pump(o));
    assert(video_audio_poll(h).position_frames == paused);
    delay();
  }
  assert(held.frame.frame.bitmap.revision == revision);
  assert(video_audio_seek(h, 57600));
  assert(video_decoder_seek(v, 2, 1.2));
  assert(video_decoder_release(v, &held));
  have = false;
  prefill(h);
  assert(video_audio_poll(h).position_frames == 57600);
  assert(video_audio_play(h));
  for (int i = 0; i < 10000 && !video_audio_poll(h).ended; i++) {
    assert(video_sdl_pump(o));
    frame(v, 2, video_audio_poll(h).position_frames / 48000.0, &held, &have);
    delay();
  }
  assert(video_audio_poll(h).ended);
  assert(video_audio_poll(h).position_frames == 96000);
  assert(have);
  for (int i = 0; i < 1000 && !video_decoder_poll(v).eof; i++) {
    frame(v, 2, 2.0, &held, &have);
    delay();
  }
  assert(video_decoder_poll(v).eof);
  assert(video_ffmpeg_poll(a).video_packets == 20);
  assert(video_decoder_release(v, &held));
  assert(video_audio_destroy(h));
  video_ffmpeg_destroy(a);
  assert(video_sdl_destroy(o));
  assert(video_decoder_close(v));
  printf("PASS combined real FFV1+PCM, dummy mixer clock, red/blue due frames, "
         "pause, seek57600, EOF96000; SDL %s\n",
         SDL_GetRevision());
  SDL_Quit();
  return 0;
}
```
