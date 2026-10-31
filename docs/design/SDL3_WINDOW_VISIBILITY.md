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
