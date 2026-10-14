# Opt-in Popover source appearance

This slice implements consumed surface appearance against gpui-component 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/styled.rs:16–125,186–194` and `crates/component/src/popover.rs:355–361`. It does not complete the Popover family.

Append `popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE` to `cui_surface_options` for a POPOVER. Zero/LEGACY preserves the existing surface background, text, padding and 1px border with no shadows. SOURCE uses theme popover/popover_foreground, theme radius, 12 logical pixels padding (p_3 at the pinned 16px rem baseline), no ordinary border and these ordered DROP layers:

| x | y | sigma | spread | ink |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | 1 | theme text RGB, alpha26 |
| 0 | 4 | 3 | -1 | black alpha26 |
| 0 | 2 | 2 | -2 | black alpha26 |

The first layer is an expanded filled rounded caster, not an outline. Original caster radii are retained before spread. Core copies the three layers onto the actual portal node; ancestor opacity is applied by Core once. Layer alpha is the nearest byte to 0.1. This is numerical CUI raster parity, not certification of GPUI's GPU kernel/AA/color identity.

NONE skips background, padding, radius, border and shadow defaults. Explicit refinement flags apply afterward in every policy, including NONE: background/foreground/radius/padding/border and a replacement ordered list of up to eight mixed DROP/INSET layers. A zero-count SHADOWS refinement explicitly removes default shadows. Values are owned copies; no layer pointers remain borrowed. Unflagged properties do not override defaults. Unknown flags/policy, count overflow, invalid finite geometry or invalid layer kind/geometry reject before Base input reduction or decision callbacks. The pure `cui_popover_style_resolve` leaves output untouched on failure. Styled and Base begin return false for invalid Popover configuration. Other surface kinds ignore these Popover-only options.

Host rem changes require an explicit padding refinement; theme font size is not a rem proxy. CUI retains its automatic title/Close/body composition, whereas source Popover content is arbitrary caller content. NONE uses theme text as the fallback for that explicit title; source would inherit the enclosing element's foreground. An explicit foreground refinement overrides this fallback. Body children keep their caller-specified colors. General inherited text styling, arbitrary source StyleRefinement, arrows/anchor policy and appearance-off parent-color identity are not completed here. Appearance does not alter outside-dismiss policy. Public Popover does not receive an invented 150ms animation: the source's private dropdown helper is a separate consumer.

## Tests and registration

Root registers these new files; this worker does not edit global build or distribution files:

* `tests/unit/test_popover_appearance.c`: strict composed Core/Popover commands, light/dark distinct semantic colors, exact caster/order/radii, legacy and NONE custom shadows, resolver invalid geometry/count/kind, pending-dismiss callback/state atomicity.
* `tests/integration/test_popover_appearance_native.c` with `popover_appearance_fixture.h`: value-copy actual styled commands before destroying their Core context; identical TEXT omission for geometry/alpha proof; Software/SDL Renderer/SDL GPU/Sokol readback at DPR1/2, light/dark opaque/translucent surfaces. Does not claim font pixels. Link cui_software, cui_sdl3, cui_sdl_gpu, cui_sokol and their native dependencies; define GL_GLEXT_PROTOTYPES. Needs a real Vulkan/GL display. Utility GPU window is constrained square because tiling WM resizing would otherwise introduce intentionally unsupported anisotropic scale.
* `uv run tests/web/test_popover_appearance.py --output OUTPUT --chromium PATH`: pinned Playwright1.58, Zig0.16 reactor, actual composed command exports, Canvas/WASM shadow values/pixels and legacy/NONE absence at DPR1/2. This is a standalone fixture, not gallery wiring.

Strict native build passes. Full baseline CTest162/163 has only expected stale generated `distribution` failure; Root regenerates after integration. Actual native and Chromium1208 fixtures each pass8 source frames. Native readback compares straight interior/outside points within three byte levels to the independently scaled Software reference; corner AA identity is not promised. First plain-Xvfb Vulkan failure and real-display anisotropic fixture failure were retained at `/tmp/cui-popover-native.log` and `/tmp/cui-popover-gpu-debug.log`; fixed real-display proof is `/tmp/cui-popover-native-display.log`. Browser log `/tmp/cui-popover-browser.log`. Scoped ASan/UBSan with leak detection passes (`/tmp/cui-popover-asan-final.log`); actual Wasmtime49 WASI headless assertions pass (`/tmp/cui-popover-wasi-final.log`). Rust is not used by this slice. The opacity test confirms half-opacity ink13 across all three layers.
