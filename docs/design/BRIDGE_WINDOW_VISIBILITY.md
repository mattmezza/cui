# Bridge hidden-window contract checkpoint

Keep existing cui_bridge_options layout, ABI version, constructor and visible
creation defaults. Add cui_bridge_window_options containing uint32_t struct_size,
version (1), initially_hidden (0/1), a size query, and a separate
cui_bridge_create_with_window_options(base, window, status). NULL window options
mean visible defaults. Validate the independent record before allocating.
Headless hidden=1 returns BACKEND_UNAVAILABLE; zero/default headless is unchanged.
SDL receives the hidden flag in its constructor, before any frame.

Add set_window_visible(app, uint32_t) and get_window_visible(app, uint32_t*).
Invalid output pointers/flags fail, and errors preserve query outputs. Existing
bridge_ready rules apply: NULL invalid, closed CLOSED, active stepping invalid.
Use only the owning native UI thread. After-frame run dispatch may change/query
visibility because Core stepping has ended. No window is shown by run or step.
Rendering-only Sokol/SDL_GPU hosts remain caller-owned and outside this bridge.

Python initially_hidden, JS/TS initiallyHidden, Go InitiallyHidden, and Rust
WindowOptions/new sdl_with_window_options retain existing visible defaults.
Expose set_window_visible/window_visible (Go SetWindowVisible/WindowVisible).
Python/Go resolve new symbols optionally so default apps still load older ABI1
libraries; feature requests return unavailable without a show-then-hide fallback.
Native proof checks actual SDL hidden flags before and after the first frame,
explicit visibility transitions, lifecycle guards and all five language runtimes,
including separately compiled and executed TypeScript, only under private Xvfb.
