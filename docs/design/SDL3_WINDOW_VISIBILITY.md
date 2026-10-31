# SDL3 initial window visibility

Add `cui_sdl3_create_with_options` and a new `cui_sdl3_window_options` value
containing `bool initially_hidden`. Zero preserves visible creation. Keep the
existing constructor and its ABI unchanged as a wrapper. Set SDL_WINDOW_HIDDEN
in SDL_CreateWindow itself; never create visible then hide. Add main-thread
visibility query and explicit show/hide operations with null-safe failure.

SDL3 owns the window. Sokol and SDL_GPU adapters borrow host resources, so their
callers control initial visibility. Bridge/language native runners keep visible
creation for now; this additive C host API does not imply binding parity.

Test real hidden creation, explicit show/hide, zero defaults and legacy creation
under private Xvfb using the local wrapper. No desktop windows. Other window
systems remain unverified. No existing by-value options structure changes.

The bridge currently receives `cui_bridge_options` by pointer, with its fields
mirrored by multiple FFI declarations. The existing record requires exact struct size and ABI version. A compatible
future extension can add a separate `cui_bridge_create_with_window_options` entry point and a fixed-width,
versioned options record, retaining existing options layout and create behavior.
A post-create setter alone cannot prevent the initial flash. Builders must pass
that option before the bridge native constructor and expose explicit visibility
controls before hidden authoring can be claimed. That work is deferred here.

Validation: warning-clean focused C11 build on Linux with SDL 3.4.18. Private-Xvfb
CTest passed `sdl3_window_visibility`, `sdl3_system_appearance`, and
`sdl3_services`. The visibility test checks the actual SDL hidden flag before
any explicit show, show/hide transitions, null controls, zero options and the
legacy visible constructor. The constructor source passes SDL_WINDOW_HIDDEN
at creation. Other native window systems and GPU hardware are not validated.
