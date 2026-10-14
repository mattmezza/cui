import os
from cui_bridge import App, button, column, label

count = 0
status = label("Clicks: 0")
def increment(event):
    global count
    count += 1
    status.label(f"Clicks: {count}")

click = button("increment").label("Click me").on_click(increment)
with App(backend="sdl", title="CUI Counter", font_path=os.environ["CUI_FONT"]) as app:
    app.mount(column().children(status, click))
    if os.environ.get("CUI_EXAMPLE_SMOKE") == "1":
        app.step()
        x, y, w, h = app.bounds(click)
        app.queue_pointer("down", x + w / 2, y + h / 2)
        app.queue_pointer("up", x + w / 2, y + h / 2)
        app.step()
        app.dispatch()
        assert count == 1
        app.step()
    else:
        app.run()
