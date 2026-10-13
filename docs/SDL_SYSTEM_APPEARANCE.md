# SDL system appearance observation

`cui_sdl3_appearance` returns a copied, portable `cui_platform_appearance` on
 the main thread. Creation queries the real SDL system theme and starts revision
at 1. `cui_sdl3_poll` consumes system-theme notifications, re-queries SDL, increments
the revision (saturating at UINT64_MAX), and requests Core redraw. Idle polling
keeps the revision stable. Notifications contain no theme payload; repeated
notifications may produce the same appearance with a new revision. Poll retains
its usual continue/quit return contract. No callback or UI pointer is retained.

`CUI_APPEARANCE_SYSTEM_THEME` indicates API/notification availability, including
when the answer is `CUI_SYSTEM_APPEARANCE_UNKNOWN`. Unknown does not mean light:
the application must choose its own fallback or retain its last known choice.
There is no reduced-motion or high-contrast OS detection in this seam. Those
preferences remain explicit application inputs. The adapter has no Theme link
and does not change an application's explicit light/dark choice.

SDL has a process-global event queue. The backend which polls a notification
consumes it; this is not per-window multicast. Applications with several SDL
backends should designate an event owner and distribute its copied appearance
snapshot to other windows. Revisions are local to each backend instance and reset
on recreation; do not compare them across instances or after saturation.

A native Theme application can map the observation explicitly:

```c
/* After normal cui_sdl3_poll(host, ui), on the main thread. */
cui_platform_appearance appearance;
if (cui_sdl3_appearance(host, &appearance) &&
    appearance.revision != last_observation_revision) {
    last_observation_revision = appearance.revision;
    if (appearance.appearance != CUI_SYSTEM_APPEARANCE_UNKNOWN)
        preferences.dark = appearance.appearance == CUI_SYSTEM_APPEARANCE_DARK;
    /* Application revision also accounts for explicit preference changes. */
    if (preferences.revision != UINT64_MAX) preferences.revision++;
    cui_theme_host_update(&resolved, &registry, selection, preferences, 16);
}
```

Initialize the caller's revision sentinel to zero for first resolution; define
an explicit unknown fallback in `preferences.dark`. `selection.mode` may remain
`CUI_THEME_FORCE_LIGHT`/`CUI_THEME_FORCE_DARK`: observation updates the system
preference input but does not overwrite that selection. Registry/Theme headers
and linkage are only needed by the application using this example.

Provenance: repository SDL fetch pin
`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`,
[SDL video source](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/video/SDL_video.c)
updates system state before sending the notification;
[pinned event definitions](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/include/SDL3/SDL_events.h)
provide a common event without appearance data.
[Official API documentation](https://wiki.libsdl.org/SDL3/SDL_GetSystemTheme)
requires main-thread use and SDL 3.2.0 or newer. Validation uses installed SDL
3.4.18. Driver/desktop support varies; this does not claim every OS discovers a
known theme, or validate a physical desktop preference change.

`tests/integration/test_sdl3_appearance.c` uses actual SDL dummy/software drivers,
compares the initial observation with `SDL_GetSystemTheme`, queues two real SDL
notifications, and checks monotonic revision/redraw, idle stability, null-output
atomicity and destruction/recreation. It does not fabricate light/dark data.
Integrator: register this test linked to `cui_sdl3` and SDL3; default builds remain
unaffected when SDL is disabled. Generated distribution remains integrator-owned.

## Compiled native consumer

`examples/system_theme/main.c` is a complete application, defaulting to Follow
System with a light fallback for an initially unknown appearance. Later unknown
observations retain the last known preference. Two custom named themes are loaded
through the actual ThemeSet importer into owned bounded registry entries. Sidebar,
content, palette status and three real buttons expose Follow System, Always Light
and Always Dark. Notifications update the system preference while retaining an
explicit mode choice; selecting Follow System later uses the newest observation.
Application state must stay at a stable address because its registry borrows inline
entry storage. State transitions reject stale generations, invalid enums and
revision overflow without applying partial changes.

The static scene blocks on `SDL_WaitEvent(NULL)` when Core needs no redraw. SDL
leaves the notification queued for the adapter to dispatch; there is no periodic
polling spin. A finite `--frames` mode deliberately renders consecutive frames
for reproducible smoke/capture and does not wait. An application adding timed
animations must replace indefinite waiting with its earliest animation deadline.
No reduced-motion/high-contrast OS values are manufactured.

Example manual build from the configured native tree (root owns target registration):

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  $(pkg-config --cflags sdl3) examples/system_theme/main.c \
  build/libcui_sdl3.a build/libcui.a build/libcui_text.a build/libcui.a \
  build/libcui_bidi.a $(pkg-config --libs sdl3 harfbuzz freetype2) -lm \
  -o build/system_theme
build/system_theme --font /usr/share/fonts/noto/NotoSans-Regular.ttf
xvfb-run -a build/system_theme --font /usr/share/fonts/noto/NotoSans-Regular.ttf \
  --frames 3 --capture build/system-theme.bmp
```

`tests/integration/test_system_theme_app.c` compiles the same original example
state/frame functions with its native entry point disabled. It tests actual
Core button activation, palette changes, unknown/stale/invalid observations,
explicit modes and bounded revision failure. Link it to `cui`; SDL headers are
required because it includes the native example, but SDL runtime linkage is not
required by this headless test. Native Xvfb smoke verifies the compiled renderer
and actual font capture; a physical desktop preference event remains outside this
validation. The adapter's queued SDL notification test covers the notification path.
