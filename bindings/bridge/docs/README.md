# CUI bridge v1: C ABI and Python builders

This slice exposes real CUI row, column, label, and Button components through a portable C11 shared-library ABI. Python retains a convenient widget tree; each `step()` submits that tree to Core and renders a frame. The bridge owns copied UTF-8 strings, stable CUI IDs, Button state, and a bounded click queue. Callbacks run during `dispatch()`, after the C frame returns. `step()`/`poll_events()` allow integration with another event loop.

Build privately from the repository root:

```sh
cmake -S bindings/bridge -B build-bridge -G Ninja
cmake --build build-bridge --target cui_bridge test_bridge_core
ctest --test-dir build-bridge -R '^bridge_' --output-on-failure
```

Set `CUI_BRIDGE_LIBRARY` to the built shared library and `PYTHONPATH=bindings/python`; run `bindings/bridge/examples/save_changes.py`. The example uses SDL and requires `CUI_TEST_FONT` to point at an existing TTF. Use `App(backend='none')` for headless operation. An unavailable SDL backend or missing font reports an error; it does not silently run headless.

The public header is `include/cui/cui_bridge.h`. Call `cui_bridge_abi_version()` and `cui_bridge_options_size()` before constructing `cui_bridge_options`; set `struct_size` and `abi_version`. Destroy every app with `cui_bridge_destroy()`. Node handles contain slot and generation and belong to the app that created them. C callers must preserve that provenance; Python rejects cross-app widgets and use after disposal. All calls for one app must occur on one thread. Strings must be NUL-terminated UTF-8, up to 4096 bytes excluding NUL; setters copy them. Width and height must be finite logical pixels in `(0, 32768]`. Nodes and pending events have configurable fixed capacities (defaults 256, maximum 1024 nodes and 4096 events); errors do not overwrite output handles, bounds, or events.

The design size setter maps to CUI's complete `inherit`, `xs`, `sm`, `base`, `md`, `lg`, `xl`, `2xl` through `9xl` scale; Button defaults to `base`. Python `.gap(2)` and `.gap_px(2)` mean two logical pixels. `.gap_units(2)` means eight logical pixels (four pixels per unit). `.rounded_md()` selects the theme's medium Button rounding. Custom hover RGBA maps to existing primary, secondary, or outline Button palettes; ghost custom hover is unsupported. Unsupported props are rejected explicitly.

This is a narrow first slice, not a binding of all 78 manifest components. Native SDL requires an explicit font path and its existing renderer; headless metrics are approximate. Events are bounded and must be drained with `poll_events()` or `dispatch()`. Python callback exceptions propagate after the C frame and can be handled by the caller. The current C ABI is not a layout engine or arbitrary CSS API, and the bridge makes no full GPUI parity claim. Windows DLL exports are annotated, but Windows runtime behavior has not been validated here.
