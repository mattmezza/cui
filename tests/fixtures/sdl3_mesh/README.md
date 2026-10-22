# Private SDL mesh proof fixture

Linux-only linker-wrap fixture; no public SDL handles or test hooks. Uses existing
SDL/FreeType/HarfBuzz packages and an existing font. Do not install dependencies.
Tests run through the repository wrapper's private Xvfb, with SDL's real software
renderer. Most frames keep the target unpresented for defined actual SDL readback;
the final frame also calls real SDL_RenderPresent.

```sh
cmake -S tests/fixtures/sdl3_mesh -B build-sdl-mesh -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdl-mesh --target test_sdl3_mesh test_mesh_cpu test_render
python scripts/test_local.py --build-dir build-sdl-mesh -R '^(sdl3_mesh|mesh_cpu|render)$' --output-on-failure
```

Use a separate Debug build with `-DCUI_SANITIZE=ON`. To select an existing pinned
SDL prefix, configure with its `PKG_CONFIG_PATH`; use that prefix's library path
when running, and inspect the CMake cache/runtime linkage. No fetch is required.

The device-space oracle is a copy of the pre-extraction software algorithm with
only a double axis projection added; it does not include the shared CPU helper.
It creates one full-target texture per triangle independently. Compare actual SDL
readback exactly against the adapter's cropped textures, including transparent
clears/targets. Both use the existing SDL texture blend contract. SDL point drawing
is not the reference: on the local software window target it writes different
stored alpha than texture blitting. Raw straight-alpha software target
storage is likewise not a valid transparent SDL-target comparison.

Allocation/surface/upload/configuration failures are injected only in adapter
calls, not inside the dynamically linked SDL allocator. Real SDL resources are
still created, destroyed and read. Retained image/glyph cache textures are counted
separately from frame textures and must all be released at backend destruction.
Tests cover post-clear submission/state failure without claiming rollback. The
fixture does not establish hardware GPU or other-platform driver behavior.

Canonical test registration, renderer capability metadata/consumer guidance and
source-resource regeneration remain integration work; this worker does not edit
those files.
