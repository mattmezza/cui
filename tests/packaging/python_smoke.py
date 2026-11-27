"""Run from an installed wheel outside the checkout; no path/env overrides."""
import os
import re
import subprocess
import tempfile
from pathlib import Path
import cui_bridge
from cui_bridge import (App, IconSource, SpinnerConfig, SpinnerEasing, button, carousel,
                        column, item, kbd_stroke, rotating_spinner, row)
assert not os.environ.get("CUI_BRIDGE_LIBRARY")
assert not os.environ.get("PYTHONPATH")
backend = os.environ.get("CUI_PACKAGE_SMOKE_BACKEND", "none")
seen = []
# This UI/install probe accepts any caller-supplied Latin font. Contextual Unicode
# and explicit fallback coverage are checked by test_installed_text.py.
widget = button("save").label("Save changes").on_click(lambda event: seen.append(event.frame_sequence))
kwargs = {"backend": backend, "title": "CUI Package Native Smoke"}
if backend == "sdl":
    kwargs["font_path"] = os.environ["CUI_PACKAGE_SMOKE_FONT"]
with App(**kwargs) as app:
    assert app.supports_radio_tab_index(), "installed bridge lacks Radio ordering"
    assert app.supports_radio_paint(), "installed bridge lacks Radio paint"
    assert app.supports_rating_options(), "installed bridge lacks Rating options"
    assert app.supports_stepper_fill(), "installed bridge lacks Stepper Fill"
    assert app.supports_stepper_wrapped_fill(), "installed bridge lacks Stepper Wrapped Fill"
    assert app.supports_virtual_list_axis(), "installed bridge lacks horizontal VirtualList"
    assert app.supports_kbd_style(), "installed bridge lacks Kbd styling"
    assert app.supports_responsive_notifications(), "installed bridge lacks responsive notifications"
    assert app.supports_virtual_list_rows(), "installed bridge lacks VirtualList live rows"
    assert app.supports_disclosure_presentation(), "installed bridge lacks disclosure presentation"
    assert app.supports_disclosure_heading(), "installed bridge lacks disclosure headings"
    assert app.supports_disclosure_divider(), "installed bridge lacks disclosure dividers"
    assert app.supports_carousel_basis(), "installed bridge lacks fractional Carousel slides"
    assert app.supports_carousel_navigation(), "installed bridge lacks Carousel navigation"
    assert app.supports_kbd_stroke(), "installed bridge lacks typed Kbd"
    assert app.supports_spinner_presentation() and app.supports_spinner_icon_resource()
    changes = []
    overview = item("overview", "Overview").carousel_basis(0.5)
    activity = item("activity", "Activity").carousel_basis(0.5)
    settings = item("settings", "Settings").carousel_basis(0.5)
    slides = carousel("slides").visual_size(240, 100).children(overview, activity, settings)
    slides.on_change(changes.append)
    hint = kbd_stroke("shortcut", "ß", modifiers=2, outline=True, accessible_name="Save shortcut")
    reference = kbd_stroke("reference", "sS", modifiers=2, outline=True)
    loading = rotating_spinner("loading", SpinnerConfig(easing=SpinnerEasing.LINEAR))
    app.mount(column().children(widget, slides, hint, reference, loading))
    artwork = app.icon_resource(IconSource.svg(
        '<svg viewBox="0 0 24 24"><path d="M12 2 L22 12 H12 Z"/></svg>'))
    loading.set_icon_resource(artwork)
    app.step()
    first = loading.presentation()
    assert first is not None and first.next_wake_seconds > first.core_time
    assert artwork.bitmap().width > 0 and tuple(app.bounds(loading)[2:]) == (16, 16)
    app.step(.08)
    assert loading.presentation().phase > first.phase
    loading.set_icon_resource(None)
    artwork.close()
    hint_width, hint_height = app.bounds(hint)[2:]
    assert hint_width > 0 and hint_height > 0
    assert (hint_width, hint_height) == tuple(app.bounds(reference)[2:])
    assert app.bounds(activity)[0] - app.bounds(overview)[0] == 128
    x, y, w, h = app.bounds(widget)
    assert w > 0 and h > 0
    if backend == "sdl":
        tree = subprocess.check_output(["xwininfo", "-root", "-tree"], text=True)
        windows = re.findall(r'(0x[0-9a-f]+) "CUI Package Native Smoke"', tree)
        assert len(windows) == 1, tree
        with tempfile.TemporaryDirectory(prefix="cui-package-pixels-") as pixels:
            capture = Path(pixels) / "frame.xwd"
            subprocess.run(["xwd", "-silent", "-id", windows[0], "-out", str(capture)], check=True)
            region = f"{int(w)}x{int(h)}+{max(0,int(x))}+{max(0,int(y))}"
            colors = subprocess.check_output(["convert", str(capture), "-crop", region, "-format", "%k", "info:"], text=True)
            assert int(colors) > 1, "native frame must contain rendered Button pixels"
    app.queue_pointer("down", x + w/2, y + h/2)
    app.queue_pointer("up", x + w/2, y + h/2)
    app.step()
    assert app.dispatch() == 1 and len(seen) == 1
    assert slides.carousel_adjacent() is activity and slides.selected() is overview
    assert slides.carousel_step() is activity and changes == []
    assert app.dispatch() == 1 and changes[0].frame_sequence > 0
    assert slides.selected() is activity
library = Path(cui_bridge.__file__).parent / "_native/libcui_bridge.so"
os.environ["CUI_BRIDGE_LIBRARY"] = str(library)
with App() as app:
    app.mount(row().children(button("override")))
    app.step()
os.environ["CUI_BRIDGE_LIBRARY"] = "/missing-cui-artifact.so"
try:
    App()
except OSError:
    pass
else:
    raise AssertionError("explicit broken override must not fall back to installed artifact")
os.environ.pop("CUI_BRIDGE_LIBRARY")
hidden = library.with_suffix(".hidden")
library.rename(hidden)
try:
    try:
        App()
    except FileNotFoundError as error:
        assert "packaged bridge is missing" in str(error)
    else:
        raise AssertionError("missing installed bridge must report a diagnostic")
finally:
    hidden.rename(library)
print("installed Python bridge smoke passed", backend)
