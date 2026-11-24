# Responsive Skeleton in Python

Use `responsive_skeleton` to show the shape of content while it loads. A
placeholder stays decorative. Keep a visible loading status and replace
placeholders with your real content when the application is ready.

```python
from cui_bridge import App, SkeletonConfig, SkeletonWidth, column, label, responsive_skeleton

avatar = responsive_skeleton("avatar", SkeletonConfig(
    width_mode=SkeletonWidth.FIXED, width=40, height=40, radius=20))
name = responsive_skeleton("name", SkeletonConfig(width=.65, radius=8))
role = responsive_skeleton("role", SkeletonConfig(width=.4, height=12,
                                                  radius=6, secondary=True))
description = responsive_skeleton("description", SkeletonConfig(height=36))

with App(width=480, height=320) as app:
    app.mount(column("profile").gap(8).children(
        label("Loading profile…"), avatar, name, role, description))
    app.step()
    assert app.bounds(description)[2:] == (448, 36)
    sample = description.presentation()
    assert sample and sample.next_wake_seconds > sample.core_time
```

The default configuration fills the parent's inner width and is 16 logical
pixels tall. Fractional width resolves against a parent with a defined width.
For a flexible width with a cap, use `SkeletonWidth.GROW`, `width=0` and
`max_width=128`, for example. Fractional width requires `max_width=0`.
Fixed widths, heights, width caps and corner radii use logical pixels.

The active theme preset supplies the fill. `secondary=True` halves its alpha.
A supplied `background_rgba` replaces the theme fill entirely and is expressed
as numeric `0xRRGGBBAA`. For example, `0x297AEAFF` is opaque blue and `0` is
transparent black. The default pulse lasts two seconds, and
`reduced_motion=True` keeps the fill static.

Configurations and presentation samples are immutable Python values. Use
`dataclasses.replace(shape.get_config(), reduced_motion=True)` and pass the
result to `shape.configure(...)`. A cosmetic change preserves the motion
clock. Changing the period or reduced-motion policy restarts it. Call
`shape.remove()` to release a mounted placeholder after loading. This
permanently removes that widget instance from the mounted UI.

`presentation()` reads the sample from the latest successful frame. It
returns `None` before a sample is available, including after reconfiguration
or while a closed ancestor omits the node. In a manual loop, the delay to the
next suggested update is `sample.next_wake_seconds - sample.core_time`.
A zero deadline means the placeholder requests no wake. The native
`App.run()` loop uses its existing cadence.

## Run the profile UI

From a checkout containing the new bridge, configure the build and run the
complete [profile example](../bindings/bridge/examples/skeleton_profile.py):

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON
cmake --build build --target cui_bridge
PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" \
  python bindings/bridge/examples/skeleton_profile.py
```

Those commands use the Linux library filename. The default run stays headless
and exercises the button and the loading-to-content transition. Build with
`-DCUI_BUILD_NATIVE=ON -DCUI_REQUIRE_NATIVE=ON` and pass
`--font /path/to/font.ttf` to open the same UI in an SDL window. Click **Finish
loading** to show the profile. See the [build prerequisites](BINDING_BUILDERS.md).

The opt-in API is available when
`app.supports_skeleton_presentation()` returns true. It requires the complete
V1 symbol group and matching record sizes. Legacy `skeleton(key, text)` and
`.visual_size(width, height)` still work with the published alpha.4 library.
This responsive Python builder is integrated for a future release. Direct C
composition also offers custom easing and the full box descriptor.
