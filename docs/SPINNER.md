# Spinner

Use `cui_spinner_ex` for the frozen source-style decorative loading icon. It
paints the Loader SVG at 16 logical pixels in `theme.text` by default, rotating
through one turn every 0.8 seconds with quadratic ease in/out. Sizes 12, 14, 16
and 24 correspond to source presets at a 16-pixel rem. Existing `options.size`
remains explicit logical pixels; any positive size up to 4096 is allowed.
The application owns whether loading is happening and when to stop submission.
Spinner adds no status role, busy relationship, focus stop or completion event.
Announce the actual operation using the application's semantic tree when needed.

```c
/* Persistent application state: one instance per independent spinner. */
cui_loading_motion loading_motion = {0};

/* During an active frame: */
cui_spinner_options options = cui_spinner_options_default();
options.now_seconds = host_now_seconds;
options.size = 24;
options.custom_color = true;
options.color = (cui_color){220, 40, 90, 255};
options.reduced_motion = user_reduced_motion;
cui_loading_motion_result result;
if (cui_spinner_ex(ui, loading_id, &theme, &loading_motion, options, &result)) {
    /* Absolute deadline in host_now_seconds' clock domain; zero means idle. */
    next_wake_seconds = result.next_wake_seconds;
} else {
    /* Report submission failure; inspect the Core frame error as appropriate. */
}
```

Keep motion state between frames. Initial zero state starts at the first
successful supplied time; explicitly zero it to restart. Time is finite [0,1e12],
rewinds clamp to the last sample, and duration zero selects 0.8 seconds. Explicit
durations must be finite [0,60]. Context or option reduced motion samples raw
phase zero, **calls custom easing at zero**, and returns inactive/deadline zero
with no perpetual invalidation. A shifted easing therefore freezes at its shifted
angle. This corrects the earlier `cui_spinner_ex` behavior that skipped easing
under reduced motion; a nonfinite reduced easing now fails atomically rather than
being silently bypassed. The legacy pure `cui_loading_motion_sample`, Skeleton
and the eight-dot `cui_spinner` keep their prior contracts. Moving results request the next sample at 60 Hz. Hidden
or clipped submission does not automatically cancel application scheduling; omit
hidden spinners when appropriate. Stop scheduling when the operation completes.

## Named rem sizes and custom pixels

An optional descriptor preserves the source distinction between named rem sizes
and fixed custom pixels:

```c
cui_spinner_sizing sizing = {.size = CUI_SPINNER_LARGE, .rem_size = 20};
options.sizing = &sizing; /* Large = 1.5 * 20 = 30 logical pixels. */
/* Or fixed pixels, unaffected by rem or theme font changes: */
sizing = (cui_spinner_sizing){.size = CUI_SPINNER_CUSTOM, .pixels = 64};
```

XS/S/M/L multiply rem by .75/.875/1/1.5. A zero named `rem_size` selects 16 logical
pixels; applications pass their root rem explicitly. This is independent of the
theme's text font size. Named rem must be finite [0,4096] and the resolved size
must be in (0,4096]. CUSTOM uses only finite `pixels` in (0,4096]; its rem is ignored.
When `sizing` is non-NULL it overrides `options.size`, even if the latter is
invalid. NULL retains the old explicit-pixel/default behavior. No automatic
ambient rem scope is introduced. The descriptor is copied synchronously and not
retained; keep it alive through the call. Custom paint callbacks receive the
same resolved logical size as the default Icon painter.

`cui_spinner_options` grew: rebuild libraries, generated distributions and callers
with the same headers. Zero-initialize options or use `cui_spinner_options_default`;
old positional/binary layouts must not be mixed with new builds.

## Custom icons and styling

A synchronous paint callback receives the selected size, color and center
rotation in radians. Use any available named icon or a caller-owned Icon state.
Host-loaded icons retain their existing loader/asset lifetime requirements.

```c
static bool paint_loading_icon(cui_context *ui, cui_id id, const cui_theme *theme,
    float size, cui_color color, float radians, void *user) {
    (void)user;
    return cui_icon_named(ui, id, theme, cui_string("loader-circle"),
        (cui_icon_options){.size = CUI_ICON_CUSTOM, .custom_size = size,
            .custom_color = true, .color = color,
            .custom_transform = true,
            .transform = cui_icon_rotation(radians)}) == CUI_ICON_OK;
}
/* Before submission: */
options.paint = paint_loading_icon;
```

`paint_user` and `ease_user` are borrowed only for the call and never retained.
`ease(phase,user)` receives normalized raw phase and may return **any finite
number of turns**, including negative turns and overshoot. `result.phase` keeps
that value; the painter receives its equivalent rotation reduced to [0,2π)
before multiplication, so even very large finite values cannot overflow radians.
At large float magnitudes fractional turns may already be lost to float precision.
NaN and infinity reject before paint. The shared legacy sampler and Skeleton
still require finite [0,1] easing and skip easing under reduced motion. A
failed easing/paint call preserves motion state/result; paint callbacks must
leave their own submission unchanged on failure. Core capacity errors retain
Core's existing failed-frame contract. Borrowed bitmap data in successful draw
lists expires at the next frame as with other Icon commands.

Compose a parent `cui_box` for placement, padding, backgrounds, borders, opacity
and clipping. Supported Icon style fields can also be supplied inside a custom
callback. Icon padding and borders floor the allocated dimensions while the SVG
paints the complete box; see the Icon guide for numeric constraints. Arbitrary
inherited source Icon styling remains an explicit parity limitation.
This guide does not claim complete Icon styling or complete renderer/platform
parity from Spinner tests. Parent composition also supports content-inset padding.
Explicit transparent color remains transparent; it is not a fallback sentinel.

## Source boundary and legacy API

The reference is GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/spinner.rs`
and its `Icon`/`Sizable` implementations. Spinner exposes icon, color, easing
and size. Its source duration is private; CUI additionally exposes a bounded
duration option. Spinner itself has no general Styled implementation, but its
custom Icon can carry style. Default easing comes from pinned GPUI 0.3.8.

The older `cui_spinner` is a compatible eight-dot phase ring with legacy Loading
status semantics and continuous invalidation while moving. It remains available;
use `cui_spinner_ex` for the source-style SVG, decorative semantics and explicit
wake scheduling. The two APIs intentionally have distinct contracts.

## Gallery and verification

The existing **Spinner** gallery entry offers source preset/custom sizes,
Loader/Loader circle/Arrow right, theme/custom/transparent colors, and default
quadratic/linear easing. Optional size-identity/rem selectors exercise the named
descriptor while legacy pixels remain the default. Additional easing choices
show quarter, negative/extra turns, shifted linear easing and deliberate nonfinite
rejection. Copy its concrete C example and open this guide from the component.
No extra catalog family is introduced.

`tests/unit/test_spinner_contract.c` verifies all source pixel sizes plus custom
64, default color, transparent color, actual rotated custom SVG pixels, static
reduced-motion restoration, supported Icon background/radius styling, ancestor
opacity and clipping, and decorative/quiescent behavior. Existing
`test_status_motion.c` covers clocks, callbacks, custom easing and rejection.
`tests/web/test_spinner_gallery.py` loads real WASM and exercises controls,
bitmap/pixel changes, reduced-motion quiescence, desktop/390px and DPR 1/2,
copyable usage and guide navigation. Native tests run through
`python scripts/test_local.py --build-dir <build>`; browsers stay headless.
These tests are evidence for the listed behavior, not hardware/GPU acceptance.

The native contract regression additionally compares real asymmetric Arrow right
pixels for quarter-turn/1.25-turn/negative equivalent turns, opposite turns and
large finite whole turns. It checks reduced custom ease(0), nonfinite failure
atomicity, live-to-reduced transitions and named rem/default/custom pixel sizing
through both default and custom Icon painters. Actual WASM now covers these descriptor and finite-turn cases in
`tests/web/test_spinner_contract_gallery.py`: desktop/390px, DPR 1/2, asymmetric
Arrow-right bitmap and canvas equivalence for .25/−.75/1.25 turns, reduced
shifted ease(0), NaN/±Infinity rejection without painter or state/result commit,
recovery and no stale wake after rejection. Named rem 0/16/20/32 is checked through
both default/custom painters, while CUSTOM and NULL-descriptor pixels stay fixed.
Logical layout size is exact. Rotated bitmap bounds may include a one-pixel
raster-rounding fringe. Independently measure the layout bounds when checking
rem sizes.

## Use a rotating icon in a retained C UI

Create a Spinner in a retained Row or Column and opt into the V1 presentation.
This example places an arrow next to your application's loading label. Omit
`icon` for the default Loader.

```c
cui_bridge_node spinner;
cui_bridge_status status =
    cui_bridge_node_create(app, row, CUI_BRIDGE_SPINNER, "sync-spinner", &spinner);
if (status != CUI_BRIDGE_OK)
    return status;

const cui_bridge_spinner_config_v1 style = {
    .struct_size = sizeof style,
    .schema_version = CUI_BRIDGE_SPINNER_V1,
    .flags = CUI_BRIDGE_SPINNER_CUSTOM_COLOR,
    .size = CUI_BRIDGE_SPINNER_LARGE,
    .rem_size = 20, /* 30 logical pixels */
    .duration = 1,
    .rgba = 0x3B82F6FFu,
    .easing = CUI_BRIDGE_SPINNER_LINEAR,
    .icon = "arrow-right",
};
status = cui_bridge_spinner_configure_v1(app, spinner, &style);
if (status != CUI_BRIDGE_OK) {
    (void)cui_bridge_node_remove(app, spinner);
    return status;
}
```

The [complete C file-sync example](../bindings/bridge/examples/spinner_status.c)
adds the Spinner next to its status label when you click Start sync. Stop sync
removes it. From the repository root, use a C11 compiler, CMake and Ninja.

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON -DCUI_BUILD_NATIVE=OFF
cmake --build build --target bridge_spinner_status_example
./build/bridge_spinner_status_example
```

This runs a headless smoke check that clicks Start sync and Stop sync. To open
a window, configure with `-DCUI_BUILD_NATIVE=ON` and provide an existing
TrueType/OpenType font as the first argument. The native bridge links SDL3,
FreeType and HarfBuzz through CMake. See the
[compiler and dependency guide](BINDING_BUILDERS.md#dependencies-and-compiler).

The default color comes from the current theme's text
color. A custom RGBA value replaces it, including a fully transparent value.
Use `CUI_BRIDGE_SPINNER_CUSTOM` with `pixels` and leave `rem_size` zero for a fixed
size. Named sizes use `rem_size` and leave `pixels` zero. Rem zero selects
16 logical pixels. Easing zero selects quadratic in/out. Duration zero selects
0.8 seconds. Add `CUI_BRIDGE_SPINNER_REDUCED_MOTION` for a static icon. Query
`cui_bridge_icon_query_v1` to check whether your build embeds a catalog icon.

The app copies the full configuration and owns the clock per node. Initialize
both header fields for configuration and presentation reads. Use these APIs on
the app's owner thread between steps. Replacing size, color, easing or icon
preserves elapsed motion. A changed effective period or explicit reduced-motion
flag restarts it. A node that reappears after an omitted frame starts at zero.
Removal releases the per-node configuration and clock.

After a successful `cui_bridge_step`, `cui_bridge_spinner_get_presentation_v1`
copies the frame sequence, time, eased turns and absolute wake deadline. A zero
deadline requests no animation wake. The query returns `CUI_BRIDGE_EMPTY` before
the first sample, after reconfiguration, after omission or after a failed step.
Elapsed time can advance if the renderer fails after composition. The deadline
is for manually stepped hosts. The native runner keeps its current cadence.
Keep the loading label and any accessibility announcements in the application.

Dynamic consumers can probe `cui_bridge_spinner_versions` and both V1 size
functions. Existing unconfigured Spinner builders keep their eight-dot ring,
legacy visual size and Loading status. Configured nodes use the V1 size
fields. Immediate C supports application paint and easing callbacks. The
retained C route supports embedded catalog icons and two built-in easing curves.
Raw SVG resources in this route, inherited Icon styling and broader host
acceptance remain open. The
[headless software check](../tests/unit/test_bridge_spinner.c) verifies real
rotation pixels, sizing, color, reduced motion, omission, failure and handle
reuse.

## Use Spinner from a language builder

Opt into the rotating Icon builder. The examples place an indicator beside
application content, then remove it when a button callback completes the
simulated operation. Each runs headlessly by default and also accepts a font
path for an SDL window.

| Language | Opt-in factory | Run from the repository root |
| --- | --- | --- |
| Python | `rotating_spinner(key, SpinnerConfig(...))` | `PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" python bindings/bridge/examples/spinner_status.py` |
| JavaScript | `rotating_spinner(key, options)` | `CUI_NODE_ADDON="$PWD/build-node/cui.node" node bindings/node/examples/spinner-status.cjs` |
| TypeScript | `rotating_spinner(key, options)` | `CUI_NODE_ADDON="$PWD/build-node/cui.node" node bindings/typescript/examples/spinner_status.ts` |
| Go | `cui.RotatingSpinner(key, config)` | `cd bindings/go && go run ./examples/spinner_loading` |
| Rust | `app.rotating_spinner(parent, key, config)?` | `cd bindings/rust && cargo run --example spinner_loading` |

Build the current C bridge or Node addon first using the
[builder setup guide](BINDING_BUILDERS.md#dependencies-and-compiler). For native
mode, build those libraries with SDL enabled and pass `--font PATH` in Python
or `PATH` as a positional argument in Node and TypeScript. In Go, add
`-tags cui_native` before the example directory and append `--font PATH`.
In Rust, add `--features native` before `--example` and append `-- --font PATH`.

```python
from cui_bridge import SpinnerConfig, SpinnerEasing, SpinnerSize, rotating_spinner

loading = rotating_spinner("sync-spinner", SpinnerConfig(
    size=SpinnerSize.LARGE, rem_size=20, easing=SpinnerEasing.LINEAR,
    rgba=0x3B82F6FF, icon="arrow-right",
))
# Add loading to your row, mount, then use loading.presentation() after a frame.
```

```js
const loading = rotating_spinner('sync-spinner', {
  size: 'large', remSize: 20, easing: 'linear', rgba: 0x3B82F6FF, icon: 'arrow-right'
});
// Add loading to your row, mount, then use loading.presentation() after a frame.
```

Python and Node use `get_config`, `configure` and `presentation` on the
Spinner. Go uses `SpinnerConfig`, `ConfigureSpinner` and
`SpinnerPresentation` on the node. Rust uses `spinner_config`,
`configure_spinner` and `spinner_presentation` on the app. Each returns
copied values. Start from `DefaultSpinnerConfig()` in Go or
`SpinnerConfig::default()` in Rust for medium Loader defaults.

Check `SupportsSpinnerPresentation()` in Go and
`supports_spinner_presentation()` elsewhere before using the new presentation
with a separately installed bridge. The probe checks the complete V1 export
group and both record sizes. Existing providers keep the ordinary Spinner.
The rotating constructor's configuration, including color, size and icon, is
subject to V1 validation. The builders support catalog icons and the built-in
quadratic and linear curves. Use immediate C for custom paint or easing.
