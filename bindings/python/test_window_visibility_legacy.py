"""A bridge with new dynamic exports hidden still supports default apps."""
from cui_bridge import App, column
with App() as app:
    assert app._api.create_window is None
    app.mount(column()); app.step()
    try: app.window_visible()
    except RuntimeError as error: assert 'unavailable' in str(error)
    else: raise AssertionError('missing optional query accepted')
try: App(initially_hidden=True)
except RuntimeError as error: assert 'unavailable' in str(error)
else: raise AssertionError('missing optional constructor accepted')
