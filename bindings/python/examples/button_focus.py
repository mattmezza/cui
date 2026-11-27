"""Opt into Button keyboard focus paint when supported by the provider."""
from cui_bridge import App, button, column

with App() as app:
    save = button('save').label('Save').on_click(lambda event: print('Saved'))
    if app.supports_button_focus():
        save.focus_treatment('outer-ring')
    app.mount(column().children(save))
    app.step()
    # Mounted Buttons can also switch to 'variant' or clear with 'legacy'.
