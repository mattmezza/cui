# Linux live-resize evidence

The callback-driven SDL3 host is tested on Linux independently of macOS modal
resize behavior. The automated test uses X11 on the private Xvfb display created
by `scripts/test_local.py`. It does not connect to the maintainer's desktop.

## What the test does

`tests/integration/test_continuous_resize.c` runs the real responsive dashboard
through `cui_sdl3_run`, using the pinned Inter font and SDL software rendering.
It presents an initial 800-by-760 frame before resizing begins. The separate
Python parent in `tests/packaging/test_continuous_resize.py` finds that process's
uniquely named X11 window through libX11 and sends a timed sequence of
`XResizeWindow`/`XFlush` requests: 360, 800, 1400, 480, 1100 and 640 pixels, repeated
four times. Requests arrive while the callback runner is already running. The
UI callbacks neither wait for the parent nor sleep/block waiting for resize.

Each callback obtains current host dimensions, submits the scene and captures its
real rendered BMP before emitting a presentation record. The parent collects
records during the burst, requiring at least six presentations and captures at
360, 800 and 1400 widths **before the burst ends**. A final-only redraw cannot
satisfy this test. It verifies:

- Window dimensions and root layout dimensions agree for every sampled frame.
- Main content stays within the current width; cards stack at 360 and sit beside
  each other at 800/1400. The sidebar becomes persistent at the LG breakpoint;
  narrow frames submit Menu and omit the closed navigation subtree.
- Captured BMP dimensions match that frame's dimensions. Actual pixels sampled
  inside both current card rectangles equal the theme's card color; screenshots
  also contain rendered UI variation. This checks fresh scene geometry against
  raster output rather than accepting a stretched old frame.
- The original Reports page, signed-in account and closed-drawer state survive
  all breakpoint crossings.

The baseline `5a76db06` passed: **24 actual presentations at six distinct widths
while a 1.46-second burst was ongoing**. This scoped callback-runner path did not
require a production fix. It does not establish that an older manual loop or
another host integration handles the user's resize path correctly.

## Run safely

After the integration target is registered, configure native SDL3/text support
and use the isolated wrapper:

```sh
cmake -S . -B build -G Ninja -DCUI_REQUIRE_NATIVE=ON
cmake --build build --target test_continuous_resize
python scripts/test_local.py --build-dir build -R '^linux_continuous_resize$' -V
```

The Python driver requires the wrapper's `CUI_TEST_VIRTUAL_DISPLAY=1`, a private
X11 DISPLAY and no Wayland display; unsupported platforms/drivers return 77.
The child independently checks the isolation flag. The driver selects X11 and
SDL software rendering. libX11 is loaded dynamically through Python's standard
`ctypes`; no Python package or new production dependency is introduced. CTest
applies `SKIP_RETURN_CODE 77` and a finite timeout. Temporary screenshots and
the child are cleaned up on success or failure.

Never replace the wrapper with `DISPLAY=:0` or the user's Wayland session.

## Coverage boundaries

Xvfb has no physical desktop window manager in this test. Programmatic X11
requests prove continuous external resize dispatch, current-size layout and
real presentation while the process runs. They do **not** prove physical mouse
window-border dragging, compositor-specific event scheduling, Wayland, hardware
GPU rendering, macOS live resize or HiDPI behavior. No custom window manager or
its potential autostart programs are launched. Those paths need separate
controlled evidence before claiming coverage.

See [responsive containers](RESPONSIVE_LAYOUT.md) for the layout policy and
[callback host lifecycle](LIVE_RESIZE.md) for runner versus manual-pump behavior.
