# Responsive Skeleton in the C bridge

Use this API when composing a retained UI with `cui_bridge_node_create`. Direct
frame-based C code uses [Skeleton presentation](SKELETON_RESPONSIVE.md).

Create a `CUI_BRIDGE_SKELETON` under a Row, Column, or another component that
renders its children. Opt the node into the new presentation with
`cui_bridge_skeleton_configure_v1`. Here is a half-width loading line under a
Column whose width is set by its parent.

```c
cui_bridge_node line;
cui_bridge_status status =
    cui_bridge_node_create(app, column, CUI_BRIDGE_SKELETON, "loading-line", &line);
if (status != CUI_BRIDGE_OK)
    return status;

const cui_bridge_skeleton_config_v1 style = {
    .struct_size = sizeof style,
    .schema_version = CUI_BRIDGE_SKELETON_V1,
    .flags = CUI_BRIDGE_SKELETON_SECONDARY,
    .width_mode = CUI_BRIDGE_SKELETON_FRACTION,
    .width = .5f,
    .height = 16,
    .radius = 8,
};
status = cui_bridge_skeleton_configure_v1(app, line, &style);
if (status != CUI_BRIDGE_OK) {
    (void)cui_bridge_node_remove(app, line);
    return status;
}
```

Use fraction 1 for a full-width line, fixed width and height 40 with radius 20
for a circular avatar, or grow with width field 0 for flexible space. Fixed
and grow modes accept an optional max width. Fractional mode requires max
width 0 and resolves against the parent's inner width. Height, fixed width,
max width and radius use logical pixels. Duration 0 selects the two-second
pulse. Add `CUI_BRIDGE_SKELETON_REDUCED_MOTION` for a static placeholder.

The current preset supplies the default fill. Secondary halves its alpha.
Custom color replaces the fill completely, including alpha zero. Enable it
with `CUI_BRIDGE_SKELETON_CUSTOM_COLOR` and specify numeric `0xRRGGBBAA`.
The bridge copies the configuration and owns the per-node motion. You can read
the configuration back, or replace it on the app thread between steps. A new
period or reduced-motion policy restarts the pulse. Cosmetic changes preserve
the pulse. Removing the node frees the configuration.

For a manually stepped host, query the latest sample after a successful
`cui_bridge_step`:

```c
cui_bridge_skeleton_presentation_v1 sample = {
    .struct_size = sizeof sample,
    .schema_version = CUI_BRIDGE_SKELETON_V1,
};
status = cui_bridge_skeleton_get_presentation_v1(app, line, &sample);
if (status == CUI_BRIDGE_OK && sample.next_wake_seconds > 0) {
    double delay = sample.next_wake_seconds - sample.core_time;
    schedule_next_step(delay); /* Application-specific scheduling. */
}
```

The returned times share the Core clock advanced by stepping. The query is
read-only. `CUI_BRIDGE_EMPTY` means there is no current sample, such as before
the first step, after reconfiguration, or when a closed container omits the
node. Reappearing after an omitted frame restarts the pulse. After a failed
frame, the query also returns empty. Timing can continue across a renderer
failure if composition had completed. The bridge's built-in native run loop
retains its continuous cadence. Applications own loading status, busy
semantics, and the transition to real content.

The API group is additive. Dynamic callers can probe
`cui_bridge_skeleton_versions()` for V1 and confirm both record sizes before
calling the new exports. Existing unconfigured nodes retain their dimensions,
colors, and motion. Once configured, use the V1 configuration to change size.
The [Python](PYTHON_SKELETON.md), [JavaScript and TypeScript](NODE_SKELETON.md),
[Go](GO_SKELETON.md) and [Rust](RUST_SKELETON.md) responsive builders use
this optional route. Built-in easing is available through this bridge.
Direct C composition also supports custom easing and the full Core box
descriptor.

The [headless software fixture](../tests/unit/test_bridge_skeleton.c) creates
real retained nodes and steps them through their full paint lifecycle. It tests
resize, colors, reduced motion, omission, failed rendering, and generation
reuse. Build and run it with the bridge enabled:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON
cmake --build build --target test_bridge_skeleton
python scripts/test_local.py --build-dir build -- -R '^bridge_skeleton$' --output-on-failure
```
