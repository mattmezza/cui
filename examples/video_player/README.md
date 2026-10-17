# Optional silent local-video player

This next-batch example decodes **actual encoded video** on a worker using an
explicitly optional FFmpeg host dependency. Core/Base/Components remain codec
independent. It uses the callback runner, controlled playback requests and
borrowed RGBA surfaces. No audio, networking, hardware decoding, subtitles,
recording or casting is implemented. Fullscreen is omitted because the present
portable window interface does not offer that action.

From the repository root, with the audited development libraries already present:

```sh
python examples/video_player/build.py --build-dir /tmp/cui-video-example --test
/tmp/cui-video-example/video_player /path/to/font.ttf /path/to/local-video.mkv
```

The build helper writes an external CMake harness; it changes no repository
CMake/module registration and installs nothing. It requires C11 threads, pkg-config,
CMake/Ninja, the existing native CUI dependencies and these audited FFmpeg library
versions: libavformat/libavcodec63.1.102, libavutil61.1.102, libswscale10.1.102.
Other versions require a deliberate compatibility audit/update; they are not
silently accepted. Native automated tests always run through `test_local.py` on
private Xvfb. They cannot fall back to a desktop display. The interactive command
above is for an explicit human launch.

The helper records complete `ffmpeg -version`, `-buildconf`, `-L` and library
versions in `BUILD_DIR/ffmpeg-build-audit.txt`. This machine's binary reports
FFmpeg n9.0.2, GCC16, shared libraries, `--enable-gpl --enable-version3` and GPLv3
or later. It enables many optional external codecs and disables magicyuv decoding.
The example does not vendor, download, package or redistribute those libraries.
Its MIT source license does not change the linked FFmpeg build's licensing.
Choose/review redistribution separately; do not assume every FFmpeg build has the
same license or codec availability.

## Clock, worker and frame lifetime

The UI owns confirmed timeline/view state. Controls issue requests; the example
accepts play/pause/rate and committed seek/skip requests, while begin/update/cancel
remain scrub presentation. The timeline uses the host's monotonic SDL delta;
paused time does not advance it. The worker opens/demuxes/decodes/converts local
files; no decoding, sleeping, event pumping or queue waiting occurs in UI callbacks.
Mutex critical sections copy state and manage slots, never decode or convert.
FFmpeg protocols are restricted to `file`. This example is not a hardened loader
for untrusted media or a network player.

Exactly three reusable slots bound RGBA storage, each at most4096x4096 pixels.
Slots transition FREE -> WRITING -> QUEUED -> LEASED -> FREE. The UI acquires the
latest due queued frame and may drop older queued frames. One paused lease is
retained unchanged, while the worker fills the other slots and waits. Replacement
releases the previous lease only after synchronous renderer consumption. Closing
with a lease outstanding fails; the caller must release it before joining the
worker. A copied already-released lease cannot release a reused slot because
source, generation, pointer and revision are checked. Pixel memory is never freed
or recycled at surface submission/end_frame.

Seek increments model generation before enqueueing the command. Queued old frames
are discarded; worker conversion in flight is checked against the command serial
before publication. A leased old frame stays valid until release but the player
rejects it against the new source/generation. Source identity is fixed for an
adapter instance; replacement opens a new instance with a fresh nonzero ID.
The worker seeks/flushes and discards frames before the target. Its first frame at
or after the requested target may be acquired as a paused seek preview; the host
acknowledges the actual timestamp in the confirmed view. This is frame-accurate
preview acknowledgement, not sample-accurate audio seeking.

At EOF the worker waits for a seek or shutdown. Known-duration playback ends at
the timeline bound; unknown duration uses explicit host EOF acknowledgement after
its queue drains. The last displayed frame stays leased. Errors are observable
and stop this example with failure; a production host can publish an error view
and recovery UI. Shutdown signals the worker and joins it, freeing packet/frame,
conversion context, codec/container and slot storage. A protocol interrupt callback
also observes shutdown. No asynchronous UI callback survives adapter destruction.

This simple silent clock has no buffering/rebase policy for a decoder that cannot
keep up: it may drop late frames. Audio synchronization, general edit lists,
rotation/sample-aspect/color/HDR metadata, variable-rate scheduling accuracy,
network cancellation, bounded seek latency and broad format compatibility remain
unverified. Conversion currently uses FFmpeg's default SDR RGBA conversion; it
must not be called color-managed or HDR playback.

## Evidence and boundaries

`make_fixture.py` encodes20 real64x32 red/blue frames into FFV1/Matroska using the
installed executable. `test_decoder.c` decodes that file, verifies actual RGBA and
software-render pixels, pause lease stability, seek generation/content, stale
frame rejection, exactly-once release, bounded slots, EOF and missing-file cleanup.
`test_native.py` runs the real callback player in private Xvfb, captures presented
red, paused red and post-seek blue BMPs and checks decoded interior pixels. Native
callbacks never wait for frames. No generated placeholder is substituted for a
failed decoder.

Portable unit tests separately prove controlled owner rejection, per-event
requests, copied callback snapshots, touch cancellation, keyboard seeking, focus,
accessible names/normalized ranges, accessibility button activation, SVG loader
replacement, responsive button rows, capability gating and omitted hit targets.
The legacy Base test also runs unchanged.

Browser CUI exports/bindings for the new controls are integrator-owned and are not
added by this example. Browser decoding, browser fullscreen permission behavior,
and a browser-rendered player remain **unverified**. A later browser host should
use actual HTMLVideoElement/WebCodecs output, feed copied RGBA into the same
surface, and run headless tests with a real supported encoded clip. FFV1 browser
support is not assumed. Native/software proof does not establish browser parity.
