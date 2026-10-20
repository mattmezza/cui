# Native FFV1 video with PCM sound

This optional runnable CUI example routes actual decoded/resampled PCM to SDL's
default output and presents actual FFmpeg video frames against the observed SDL
software-mixer media clock. It uses existing controls, callback-main resize,
borrowed frame leases, worker decoders and the bounded audio controller. Core
has no codec or audio-device dependency.

**Supported first profile:** immutable local Matroska with FFV1 SDR video and PCM
audio, known zero container origin, duration <=10s. Fixtures prove S16LE stereo
48k and44.1k input, normalized to48k F32 stereo. Requires obtained SDL stream
source/destination both48k F32 stereo/unity ratio. The decoder admits additional
PCM representations/rates as documented in its own README; they are not newly
verified here. Video frame durations must be known and <=1s. Audio/video endpoints
must agree within100ms; large mismatch fails explicitly at actual EOF, rather
than stretching audio or inventing a wall-time tail. Fullscreen and playback
rate controls remain omitted. This is **not general native MP4/AAC/Opus support**.
The existing silent video example and browser player remain separate paths.

## Build and automated validation

From repository root, with the already installed audited development packages:

```sh
cmake -S examples/video_player_audio -B build-video-sound -G Ninja -DVIDEO_SOUND_UI=ON
cmake --build build-video-sound --target video_player_audio test_sound test_sound_watch test_sound_output test_sound_legacy
python scripts/test_local.py --build-dir build-video-sound -- -R '^sound_'
```

The external project builds only those requested targets and their dependencies;
no root CMake or package registration is edited. Omitting `VIDEO_SOUND_UI=ON`
builds the19 no-window checks without CUI runtime linkage. Repeat with a separate
build directory and `-DVIDEO_SOUND_SANITIZE=ON` for ASan/UBSan. Native automated
smoke always requires wrapper-owned Linux Xvfb, software rendering and forced SDL
dummy audio; it cannot fall back to a desktop or physical output. Browser tests
are unrelated and remain headless.

Pins are SDL3.4.18 (runtime and pkg-config), FFmpeg CLI n9.0.2, libavformat/
libavcodec63.1.102, libavutil61.1.102, libswscale10.1.102, libswresample7.1.102.
No installation/download is performed. Generated `media/dependency-audit.txt`
records versions, FFmpeg build configuration and full license text. Tested shared
FFmpeg enables GPL/version3, so its linked package is GPLv3-or-later, not an
assumed LGPL-only build. Example source is MIT; SDL is zlib licensed. No binary
packaging or redistribution is introduced here.

## Explicit interactive launch

The test fixture generator creates a real two-second red/blue video with a
quiet generated440Hz PCM tone, plus resampling/offset/error fixtures. After the
build/test above, a human may explicitly run this command to open a native window
and send sound to their default output:

```sh
build-video-sound/video_player_audio third_party/inter/Inter-Regular.otf build-video-sound/media/fixture.mkv
```

That command is for manual use; automated agents/tests do not launch it. No
physical listening test was performed during implementation. Use the existing
font file or supply another valid font path. This example has play/pause,
committed seek/skip and replay, responsive layout and visible failures. Unsupported
media/output does not silently switch to successful silent playback. Device
invalidation or a partially accepted seek requires closing/reopening the example.

For an explicitly chosen local clip, a manual FFmpeg conversion can produce the
supported codec profile (larger, lossless files; first8s only):

```sh
ffmpeg -nostdin -i local-input.mp4 -map 0:v:0 -map 0:a:0 -t 8 -c:v ffv1 -pix_fmt bgr0 -c:a pcm_s16le -ar 48000 -ac 2 converted.mkv
```

Conversion is not automatic and requires an existing audio track. Its result must
still satisfy origin/timestamp/frame-duration/endpoint limits above; arbitrary
input edit lists, offsets and unequal tails are not promised compatible. No
compressed codec is added to the running sound adapter by this command.

## Clock, preview and failure behavior

Media time is exact acknowledged media frames divided by48000. It advances only
when the private SDL input queue actually drains, not at render/put, wall time,
callback demand or zero-filled underrun. This is **software mixer consumption**,
not backend acceptance, DAC time or acoustic playback. Downstream physical tail
is unknown; clock observation can lag by the owner-poll interval. Video frames
are selected against that observable clock, with late-frame dropping; no
speaker-level lip-sync guarantee follows.

Audio generation and video generation are explicitly mapped. Pause freezes and
reconciles output, then retains the current video lease. A committed seek
quantizes its audio target to48k frames and requests both workers. A paused video
preview may land after that target; it does not overwrite the audio target.
During playback that future preview is hidden until due. A frame borrowed during
UI composition survives a seek request in that same composition until the next
owner poll, after synchronous renderer consumption. At EOF all audio media must
drain, future video tails are validated/drained, and the last valid frame remains.

A one-sided seek failure leaves the whole session failed/paused, not rolled back
to inconsistent counters. Failed output put receives no fake consumption and
requires reset/reopen. Close destroys/joins audio before its service/output,
releases the video lease, joins video, and only then permits Core/window/font
freeing. A failed close preserves live resources for retry; example shutdown
aborts rather than free reachable userdata after unresolved quiescence failure.
OS file I/O may delay worker joins; no hard shutdown deadline or total FFmpeg
heap bound is claimed.

SDL output device added/removed/format-change events set a sticky atomic private
endpoint, including output changes unrelated to this stream. The event callback
never sees host/UI/sink pointers or calls CUI. Capture-device changes are ignored.
The next format/consumption transaction fails closed; matching format later or a
seek cannot clear it. Whole output/controller recreation is required. Owner
removes/quiesces the watch before its endpoint free using the inspected pinned
SDL dispatch/removal mutex behavior. Synthetic tests prove handling/lifetime;
real OS hotplug notification reliability remains unverified.

SDL initially queues audio enumeration internally until owner polling. The app
initializes audio, allows its normal callback-runner poll to dispatch those
startup notifications, then creates the guarded output/session. It never resets
a sticky flag or nests an event pump to hide later changes. Direct session users
must follow the same staging rule.

The owner pumps at most one1024-frame chunk per frame into a12000-frame (250ms)
queue. Callback-main remains invalidated while playing/loading/seeking. Long UI
or native modal-resize stalls can exhaust that bounded queue, producing silence;
owner resumes safely but continuous sound through arbitrary stalls is not
promised. Workers never paint, manipulate windows or pump events.

See [PROOF.md](PROOF.md) for results and intentionally deferred claims.
