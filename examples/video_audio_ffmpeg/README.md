# Optional FFmpeg PCM service proof

This standalone example supplies the existing `video_audio_decoder` interface.
It decodes real local PCM audio from muxed video, converts to 48 kHz stereo float,
and feeds the unchanged threaded controller. It has no SDL transport, speaker
output, video rendering, Core integration, playback-rate control or acoustic clock.

Build using existing dependencies, from repository root:

```sh
cmake -S examples/video_audio_ffmpeg -B build-audio-ffmpeg -G Ninja
cmake --build build-audio-ffmpeg
python scripts/test_local.py --build-dir build-audio-ffmpeg
```

Repeat in a separate build directory with `-DVIDEO_AUDIO_SANITIZE=ON` for
ASan/UBSan. Linux tests always use the private Xvfb wrapper. They generate small
local files using the already installed FFmpeg CLI; no devices, network or
installation. The fixture CLI must be FFmpeg n9.0.2. The standalone CMake project
requires exact pkg-config versions: avformat/avcodec 63.1.102, avutil 61.1.102,
swresample 7.1.102. No root build or dependency manifest is changed.

## Contract and limits

Create the service, pass `video_ffmpeg_interface(service)` as controller decoder,
and destroy the service only **after successful controller destruction/join**.
The controller's sole worker owns seek/read and all demux/codec/resampler state.
The owner may interrupt concurrently: its monotonic atomic generation ceiling
cancels older operations and does not cancel newer work. The service retains no
controller/output/UI pointer. Snapshot reads take only a short stats mutex.

Only immutable regular local files are admitted. Paths containing colon or line
breaks are rejected; FFmpeg permits only the file protocol and Matroska/WebM/WAV
demuxers. `stat` preflight is not a sandbox against path replacement. Callers must
not modify/replace a file while the service lives. Known common container origin
and audio PTS are mandatory, including for WAV; formats lacking them fail.
Nonnegative common origins only are supported in this first slice.

The admitted codecs are PCM S16LE/S24LE/S32LE/F32LE, mono/stereo, 8–192 kHz;
fixtures prove S16LE stereo at 48 kHz and 44.1 kHz. Compressed priming, skip-sample
side data, padding, edit-list interpretation, layout/format changes, missing PTS
and large discontinuities fail explicitly. This is not general FFmpeg playback.

Audio PTS and the container origin are each rescaled using rational arithmetic to
the same 48 kHz timeline. `av_rescale_delta` preserves sample continuity across
coarse packet timestamps; tolerance is one stream timestamp tick plus one sample.
Sub-tick discontinuities cannot be detected reliably. A leading audio offset up
to 250 ms produces intentional timeline media zeros: these correctly advance the
controller clock when consumed, unlike underrun padding. There is no independent
audio-origin reset. Video packets are counted/skipped, not decoded or presented.

Seek reopens and replays from the common origin, preserving the resampler phase,
then trims normalized PCM before the exact requested output frame. This proves
seek content for the supported fixtures. It is deliberately linear work, with a
10-second default timeline cap, not a fast general container seek algorithm.
Targets past EOF return EOF without inventing media. Codec EOF is drained first,
then SWR NULL-input flush is repeated until empty. Output chunks remain contiguous
and finite, with copied storage and no outstanding FFmpeg buffer leases.

Default explicit bounds: 8 MiB packet, 8192 input samples/frame, 65536 converted
spill frames, 8192 demux packets, 480000 timeline frames and 12000 leading gap
frames. Callers may lower these via `video_ffmpeg_limits`. The controller PCM ring
is separately bounded and backpressures decoder reads. Probing is configured with
a bounded probe-size hint, one-second analysis hint, packet hint and 16 streams.
These hints and post-demux packet checks are **not a total FFmpeg heap bound**:
FFmpeg may allocate packet/demux state before the service can inspect it. This is
not an untrusted-media hardening layer.

Cancellation is checked between work steps and through the AVIO interrupt hook.
An OS storage read may still block beyond the hook; no bounded wall-clock shutdown
is promised. Destroy joins rather than freeing live worker state. No sleeps,
mutexes or decoder calls run in an audio callback here; this slice implements no
audio callback. The controller mutex remains unsuitable for a hard realtime claim.

## License and evidence

Example sources are MIT. `fixtures/dependency-audit.txt` records FFmpeg version,
build configuration, license text and all four exact library versions on every
fixture run. The tested n9.0.2 shared package enables `--enable-gpl` and
`--enable-version3`, so the actual linked package is GPLv3-or-later; it must not be
represented as an LGPL-only distributable merely because SWR headers are LGPL.
No binary packaging/distribution is introduced by this proof.

See [PROOF.md](PROOF.md) for the measured coverage and deferred integration.
