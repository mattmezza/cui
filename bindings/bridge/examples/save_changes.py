"""Actual SDL Button rendered by CUI components; set CUI_TEST_FONT and CUI_BRIDGE_LIBRARY."""
import os
from cui_bridge import App, button, label, row

font = os.environ["CUI_TEST_FONT"]
save = button("save").primary().rounded_md().size("base").label("Save changes")
status = label("Ready", key="status")

def saved(_event):
    status.label("Saved")

save.on_click(saved)
with App(backend="sdl", title="CUI Python bridge", width=480, height=240,
         font_path=font) as app:
    app.mount(row().gap_units(2).children(save, status))
    app.run()
