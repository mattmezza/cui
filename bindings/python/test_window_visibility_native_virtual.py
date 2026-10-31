"""Hidden native creation and after-frame visibility; private Xvfb only."""
import os
from cui_bridge import App, BridgeError, button, row
assert os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') == '1'
assert os.environ.get('DISPLAY') and os.environ['DISPLAY'].split('.')[0] != ':0'
assert not os.environ.get('WAYLAND_DISPLAY')
try:
    App(initially_hidden=True)
except BridgeError as error:
    assert error.code == 9
else:
    raise AssertionError('headless hidden request accepted')
for hidden in (True, False):
    with App(backend='sdl', font_path=os.environ['CUI_TEST_FONT'], initially_hidden=hidden) as app:
        assert app.window_visible() is not hidden
        calls = []
        def clicked(_event):
            # Handler is dispatched after the Core frame in the native callback.
            assert app.window_visible() is not hidden
            app.set_window_visible(True)
            assert app.window_visible()
            app.set_window_visible(False)
            assert not app.window_visible()
            calls.append(1)
            app.request_close()
        target = button('show').label('Show').on_click(clicked)
        app.mount(row().children(target)); app.step()
        assert app.window_visible() is not hidden
        x, y, _, _ = app.bounds(target)
        app.queue_pointer('down', x+2, y+2); app.queue_pointer('up', x+2, y+2)
        app.run()
        assert calls == [1]
        try: app.window_visible()
        except BridgeError as error: assert error.code == 2
        else: raise AssertionError('closed visibility accepted')
print('Python window visibility PASS')
