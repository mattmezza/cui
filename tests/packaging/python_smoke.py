"""Run from an installed wheel outside the checkout; no path/env overrides."""
import os
import re
import subprocess
import tempfile
from pathlib import Path
import cui_bridge
from cui_bridge import App, button, row
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
    app.mount(row().children(widget))
    app.step()
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
