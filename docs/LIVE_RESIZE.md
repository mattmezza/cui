# Native live resize and pointer motion

Use `cui_sdl3_run(argc, argv, callbacks)` for native applications. Initialization
creates the backend and Core context; the frame callback composes and presents
one frame using freshly queried `cui_sdl3_size`; quit releases application
resources. Return zero from frame to continue, positive to finish successfully,
negative on failure. The starter and native gallery show the complete pattern.
All callbacks run on the main thread. Do not call event waits or nest a runner
inside these callbacks. `cui_sdl3_poll` remains valid there for IME/cursor
synchronization and does not recursively pump SDL.

The runner uses SDL's supported callback entrypoint `SDL_EnterAppMainCallbacks`.
The existing ordinary `main` entrypoints remain code-driven; no SDL types enter
CUI's public APIs and no extra SDLMain library is required. The existing macOS
bundle font lookup and static SDL dependency linkage remain in place.

This distinction matters during native resize. Pinned SDL 3.4.18 implements
`SDL_OnWindowLiveResizeUpdate` in `src/video/SDL_video.c`: callback-main apps get
`SDL_IterateMainCallbacks(false)` while ordinary loops only receive a queued
EXPOSED event. Cocoa's live-resize timer calls that function at approximately
60 Hz; Windows' modal size/move timer calls it too. A conventional poll loop can
remain inside the OS event pump until dragging stops, so invalidating it alone
cannot guarantee intermediate composition. The runner lets SDL invoke composition
from its main-thread modal-loop iteration, without drawing in an event watcher.
A reentrancy guard prevents nested frame callbacks. Foreign-thread lifecycle
notifications never mutate Core or draw through the adapter.

Custom poll/step loops remain supported. They must adopt callback-main scheduling
to obtain this modal live-resize behavior; calling `cui_sdl3_size` more often or
reducing a sleep interval does not replace that scheduling change. Logical resize,
pixel-size change, display-scale change and expose notifications invalidate Core.
Density-aware text rasterization, rounded-strip batching and elapsed frame timing
remain enabled. Idle callback scheduling uses SDL's event wait; active animations
keep continuous iteration. Call `cui_invalidate` for main-thread model changes or
when a frame-limited capture needs another frame.

The adapter coalesces consecutive mouse motion before forwarding it to Core's
bounded event queue. It flushes the latest position before clicks, leave, focus
and other non-motion events, preserving ordering. This prevents a burst of old
positions from filling the queue and dropping the latest hover position. It does
not coalesce touch motion or discard click transitions.

`cui_bridge_run(app, after_frame, user)` exposes the same native scheduler through
the canonical C bridge. Its optional callback dispatches language/application
events after each successful frame; return OK to continue or a status to stop.
Catch foreign exceptions before returning through C. Nested run/step returns
INVALID_STATE. Closing during the callback ends with CLOSED. Destroy during the
callback requests close and defers release until the runner exits; that pointer
must not be used afterward. Otherwise the terminal app must still be destroyed
by its owner. The native host is released at run completion. Portable builds
return BACKEND_UNAVAILABLE. Existing language `App.run` loops must call this new
symbol to benefit; retaining a sleep/step loop does not enable live modal resize.
Older bridge libraries require a clear missing-symbol/unavailable fallback.

Headless Linux coverage exercises real SDL frames after a 320x160 to 480x240
resize, main-thread callback dispatch, and a 3000-motion burst reaching the next
frame. Bridge cases cover close, callback error, reentrant calls, deferred destroy
and a null callback. These tests run through `scripts/test_local.py` in private
Xvfb. Cocoa/Windows source inspection supports the scheduling contract; actual
macOS/Windows drag responsiveness remains host acceptance, not a Linux runtime
claim. Hardware/GPU display migration remains deferred.
