# Native density integration

The integration merges origin/main `30ec050b` into package integration `81a7fe89`.
SDL keeps logical layout/input coordinates while painting at renderer density.
The additive scaled text APIs leave shaped geometry and caret coordinates logical.

The merge preserves the existing per-face glyph raster cache, elapsed frame timing,
idle event waits, and bounded rounded-shape rectangle submissions. Cache keys use
physical character size (`logical size * paint scale`); using logical size alone
would incorrectly reuse rasters after density changes. Rounded strips use device
row height while retaining batches of at most 128 strips.

Core, browser/WASI sources and SDK copies are unchanged from `81a7fe89`. SDK copies
must be refreshed by the integrator. Native tests use `scripts/test_local.py` and
its private Xvfb display. Hardware display migration and GPU density checks are
not established by virtual-display coverage.

Validation on Linux: strict default native build passed. Eight targeted native
CTest entries passed through the invisible-display wrapper: raster cache,
density, shaping, native text geometry, SDL appearance, password bridge, SDL
services and rounded batching. A separate `CUI_BUILD_BIDI=OFF` strict build passed
both density and raster-cache CTest entries. The density runs explicitly selected
`/usr/share/fonts/noto/NotoSans-Regular.ttf` through `CUI_TEST_FONT`; no selected
test was skipped. Density coverage exercises 1x, 1.5x, 2x, then revisits cached
sizes while checking stable logical metrics/carets and device-pixel coverage.
