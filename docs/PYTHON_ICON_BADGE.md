# Icons and Badges in Python

An `IconResource` owns SVG data inside an `App`. Create it from SVG bytes,
a catalog name, or a path in a resource-local asset map. Share it among
standalone Icons and Badge indicators. Each use has its own size, color,
transform and box style. A configured Badge wraps ordinary widgets, so a button
inside it keeps its own click callback.

The resource API works with the alpha.4 C library. The configured Python
builders require the current C bridge and Python source. Check
`app.supports_icon_resources()`, `app.supports_icon_presentation()` and
`app.supports_configured_badge()` when supporting multiple providers. The bridge
checks each optional symbol group and its record layouts. Existing `icon()` and
`badge()` builders continue to use catalog names and text pills.

## Run the inbox

Build the shared library from the repository root with Python 3.10+, CMake
3.20+, Ninja and a C11 compiler. Python uses standard-library `ctypes`. For a
native window, install the SDL and font dependencies listed in the build guide.
[Native build prerequisites](BINDING_BUILDERS.md).

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON
cmake --build build --target cui_bridge
export PYTHONPATH="$PWD/bindings/python"
export CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so"
python bindings/bridge/examples/icon_badge_inbox.py
python bindings/bridge/examples/icon_badge_inbox.py --font third_party/inter/Inter-Regular.otf
```

The headless invocation performs a click and checks the result. The `--font`
invocation opens the native app. Click **Mark all as read** to clear
the count without moving the button. Use the shared library path for your OS.
[Complete runnable inbox](../bindings/bridge/examples/icon_badge_inbox.py).

```python
from dataclasses import replace
from cui_bridge import (App, BadgeConfig, BadgeVariant, IconOptions, IconSource,
                        button, column, configured_badge, configured_icon)

with App() as app:
    artwork = app.icon_resource(IconSource.named("check"))
    header = configured_icon("header", artwork, IconOptions(width=28, height=28),
                             name="Workspace ready")
    status = configured_badge("status", BadgeConfig(
        variant=BadgeVariant.ICON, artwork=artwork, name="Synced",
        artwork_options=IconOptions(width=12, height=12)))
    unread = configured_badge("unread", BadgeConfig(count=8))
    mark_read = button("read").primary().label("Mark as read")
    mark_read.on_click(lambda event: unread.configure(replace(unread.get_config(), count=0)))
    app.mount(column("root").gap(12).children(
        header, status.children(button("open").label("Open workspace")),
        unread.children(mark_read)))
    app.step()
    assert status.result().submitted and artwork.info().references == 2
```

## Style, update, and reuse

- Set `IconOptions` for dimensions, design size, inherited tint, artwork opacity,
  an affine transform, box styling or constraints. `None` leaves a field
  unspecified. A supplied `0` is explicit. Colors use `0xRRGGBBAA`.
  `IconStyle` uses the C box field order for individual padding, border and
  corner values. [Icon dimensions and constraints](ICON.md).
- Choose `BadgeVariant.COUNT`, `DOT`, or `ICON`. A count of zero hides the marker.
  `maximum=0` is explicit and displays `0+` for a positive count. Use
  `named_icon="check"` or `artwork=resource` for an Icon Badge. A custom
  `name` labels the marker for assistive technology. The artwork is decorative.
- Call `configure()` with new typed options. `get_config()` reads the copied
  C descriptor for a mounted node, and returns the draft for an unmounted one.
  Failed updates leave the current config and result available. For an Icon with
  an embedded `fallback="check"`, `reset()` releases the owned resource and
  restores the legacy named image. `remove()` releases the node and its children.
- After `app.step()`, `result()` returns a copied snapshot from that step or
  `None` if the component was omitted or has no current result. `icon_error`
  is available for owned Badge artwork. The result includes a separate frame
  status. `indicator_bounds()` returns current geometry when the marker has
  valid bounds. Call `app.dispatch()` to invoke ordinary Python child callbacks
  after the frame. Reading results leaves those callbacks queued.

## Resource lifetime

Use the same app and owner thread for the resource and every consuming node.
Close a resource after removing its consumers or configuring them with a
different resource. `App.close()` also releases any remaining resources.
After a completed synchronous render, call `resource.collect()` as needed to
free inactive masks when changing SVGs, sizes or transforms repeatedly.

For path-backed artwork, `put_asset(path, svg)` supplies copied data and
`remove_asset(path)` or `clear_assets()` removes it. `set_source()`, `reload()`,
`limits()` and `info()` expose the C source and cache state. `prepare()` can
rasterize without mounting a widget. Check its returned `IconError` before
loading `bitmap()`, which returns a copied RGBA buffer. The catalog helpers
inspect embedded availability and copy SVGs for the loaded build.

The bridge's copied map handles application assets. Arbitrary render-time
loader callbacks remain available through direct C. The Python Spinner
continues to use its existing builder.
