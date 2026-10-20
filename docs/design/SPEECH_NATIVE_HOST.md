# Native Speech host contract proposal

Status: design and offline feasibility evidence; no native capture or recognizer
adapter is shipped by this proposal. Speech remains partial. The 78-entry
manifest and public ABI are unchanged.

## Evidence and exact comparison

Inspected against CUI `9b358202` and the local upstream checkout at
`87d10ae5e1299d1be18670a37570c5402f952e34` (GPUI Kit 0.7.1).
The authoritative CUI API is `include/cui/cui_speech.h`, implemented by
`src/base/cui_speech.c` and `src/components/cui_speech.c`.
Upstream reference files are `crates/component/src/speech/{recognizer,
microphone,state}.rs` and `speech/system/{mod,macos,winrt,unsupported}.rs` at
[the frozen revision](https://github.com/longbridge/gpui-kit/tree/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/speech).

| Existing CUI API | Actual behavior | Remaining host work |
| --- | --- | --- |
| `cui_speech_recognizer` | available/start/push_audio/finish/close vtable; default 16 kHz mono signed interleaved PCM16 | OS/service implementation and language configuration |
| `cui_speech_input` | start/close capture vtable requesting recognizer format | device access, permission handling, conversion, scheduling |
| `cui_speech_report_*` | bounded copied queue, UTF-8 validation, generation rejection | marshal external callbacks onto owner thread |
| `cui_speech_start/stop/cancel/dispose/pump` | session lifecycle, final drain and stop timeout | host polling/wake integration and quiescent teardown |
| `cui_speech_button/waveform` | styled control and PCM meter geometry | real audio and recognized results supplied by host |

`web/gallery.c` injects fixture reports through these same APIs. Its controls
emit PCM and text explicitly; this is not browser microphone recognition.
There is no native system recognizer fallback in CUI. Even
`cui_speech_has_recognizer` requires both recognizer and input start callbacks.
`available` is a cheap recognizer predicate, not proof of microphone permission.

Upstream uses CPAL capture plus its own conversion. macOS uses on-device-only
SFSpeechRecognizer; Windows uses WinRT dictation (online speech setting and
language pack required). Windows WinRT captures its own microphone and ignores
pushed PCM; upstream's second capture feeds only the waveform. Other platforms,
including Linux, return unsupported for the built-in system recognizer.

## Proposed optional integration boundary

Reuse SDL3 already used by the native adapter. CMake currently permits system
SDL >= 3.2 and pins its optional fetched fallback to
`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`. This environment has system SDL
3.4.18; that system version is validation evidence, not a new dependency pin.
No recognizer engine/model is present in the existing dependency set. SDL audio
conversion/capture does not perform recognition.

Keep proposed adapter files outside core/base/components. Public CUI types stay
portable C11. Adapter-private structures can hold SDL stream/device handles;
OS recognizers belong in separate Objective-C/Windows translation units. No
network client, threadpool, model download, or audio device activation belongs
in Core or in automatic tests.

The first practical runtime slice would be an SDL input provider plus an
explicit owner-thread `poll` helper. These are proposed adapter APIs, not
existing CUI symbols. It can pair with an application recognizer; by itself it
must not enable the Speech button or imply native recognition availability.

1. Provider `start` opens `SDL_AUDIO_DEVICE_DEFAULT_RECORDING` with app-side
   `SDL_AUDIO_S16`, requested rate/channels, and **no audio callback**. Publish
   a non-null capture token only after initialization/resume succeeds. An
   unsuccessful start must release every resource it acquired.
2. The UI host polls the stream in bounded whole-frame chunks, forwards copied
   samples with `cui_speech_report_audio`, then calls `cui_speech_pump` with a
   finite monotonic time. A proposed 10–20 ms active-session timer prevents an
   otherwise idle event loop from starving audio. No render loop assumption.
3. SDL performs conversion when reading the stream. Cap source backlog as well
   as per-poll work; bounded output alone does not bound SDL's internal queue.
   Treat sustained overflow as input/capacity failure, rather than silently
   dropping speech or blocking the UI thread. Determine the exact budget from
   caller workspace capacity and channel count.
4. `stop` closes capture before calling recognizer `finish`; results may drain
   while STOPPING. Poll the stop deadline even when there is no new audio.
   `cancel` closes capture/recognition without a final result and clears text.
5. `close` destroys the stream/device and invalidates its token before returning.
   No pending callback may retain access to disposed CUI state.

SDL documents the app-side format for recording and the null-callback pull
model in [SDL_OpenAudioDeviceStream](https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream).
Conversion happens in [SDL_GetAudioStreamData](https://wiki.libsdl.org/SDL3/SDL_GetAudioStreamData).
These calls are available since SDL 3.2.0. Destroying the convenience stream
also closes its device. This design uses those contracts without modifying
CUI's owner-thread report API.

## Lifetime, errors, wake and UI ownership

All CUI state/provider/lifecycle/report calls occur on one owner thread.
The report queue is not a cross-thread mailbox. An OS recognition callback
must copy into a separate bounded host mailbox, signal a host wake, and let
the UI thread drain that mailbox into `cui_speech_report_*`. Wakes need a
coalescing flag; failure to enqueue/wake must become a visible terminal error.
No worker callback may call `pump`, mutate state, or invoke event handlers.

CUI sinks contain a raw state pointer plus generation, not a weak reference.
Generation rejection protects replaced sessions only while the state allocation
remains alive. It does not make reporting after `dispose`/free safe. Teardown
must cancel work, disconnect event handlers, wait for callback quiescence or
retain a separate mailbox owner until callbacks finish, discard pending events,
and only then dispose/free state/workspace. A blocked UI thread must not wait
on a callback that itself waits for UI dispatch.

`start` can report synchronously; delivery is deferred until `pump`. Provider
callbacks must not call CUI lifecycle functions. `push_audio` must consume/copy
its borrowed sample span before returning. Event text borrows state transcript
storage and must be copied by clients needing it after the callback. Capture
and recognizer close callbacks each release their successfully returned token
exactly once. Failed-start partial handles must also be cleaned up.

Use `CUI_SPEECH_NO_INPUT` only for a known absent device,
`CUI_SPEECH_PERMISSION` only for a known permission denial, and INPUT_ERROR for
unclassified SDL failures. SDL error strings alone are not portable permission
codes. Host availability queries must not prompt for microphone access.

## OS limits and staged validation

| Host | Proposed capture | Recognition limit and required evidence |
| --- | --- | --- |
| Linux X11/Wayland | optional SDL audio backend; display-independent | no built-in OS recognizer; application provider required; do not add an unpinned model/runtime |
| macOS | SDL/Core Audio, application usage description and consent | separate Speech framework provider; guard missing usage keys, enforce on-device support/locale; test on macOS before claiming support |
| Windows | SDL/WASAPI, device/privacy policy | separate WinRT provider with apartment/event ownership; document online behavior and independent capture/waveform limitation; Windows validation required |
| Browser/mobile | outside this native slice | fixture gallery does not establish live capture/recognition support |

Before shipping the input adapter, add headless tests for conversion across
chunk boundaries, interleaved channel/frame alignment, no device, failed start
cleanup, bounded backlog, report capacity failure, cancellation/restart stale
mailbox messages, stop timeout without new events, and destruction with callbacks
in flight. Use synthetic PCM and an unbound SDL audio stream for conversion;
use injected device operations for device lifecycle failures. Never initialize
real capture in automatic tests. No fixture recognizer establishes accuracy,
privacy behavior, permission handling, or hardware microphone support.

Run warnings-enabled CMake build and `python scripts/test_local.py --build-dir
build` (Xvfb wrapper). Existing `test_speech` and `test_speech_base` cover CUI's
provider state machine, not OS host behavior. Browser checks remain headless.
Manual microphone permission, unplug/replug, OS recognizer, and real hardware
checks remain deferred until explicitly exercised on each target platform.

A small unbound-stream feasibility example can prove SDL conversion and report
handoff offline. It cannot prove native recording, permission errors, OS
recognition, or frozen Speech parity. Any eventual new consumer integration
must update source metadata and generated skill guidance and pass the agent
skill freshness/installed behavior checks.

## Reproducible offline feasibility probe

The adjacent [speech_sdl_conversion_probe.c](speech_sdl_conversion_probe.c)
is an executable feasibility fixture, not a consumer provider. On 2026-10-20 it
passed against system SDL 3.4.18:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror $(pkg-config --cflags sdl3) \
  docs/design/speech_sdl_conversion_probe.c -o /tmp/cui-speech-sdl-probe \
  $(pkg-config --libs sdl3)
/tmp/cui-speech-sdl-probe
```

It checks exact PCM16 identity across two input chunks (including signed
extremes), exact output count and drained-stream state, then converts ten
synthetic 48 kHz stereo float chunks to exactly 1600 mono PCM16 frames at
16 kHz. Different left/right constants exercise downmixing; interior samples
must remain within 100 PCM16 units of the expected .25 signal. Filter boundary
transients are deliberately excluded from that signal assertion.

No `SDL_Init`, device open, recording, network, window, or user audio is involved.
The source was formatted with the CI-pinned clang-format 23.1.2. This proves
conversion reuse only and does not replace the adapter test plan above.
A warnings-enabled full baseline build passed; skill generation produced no
changes and the 17 agent skill tests passed. The full Xvfb CTest run was stopped
at the integrator's request because runtime behavior is unchanged; the
integrator's existing suite remains the release baseline. No new production
API, native microphone support, or recognition parity is claimed.
