"""Callback runner lifecycle; only the invisible-display wrapper may launch it."""
import os
import subprocess
from cui_bridge import App, BridgeError, button, row, root_view
assert os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') == '1'
assert os.environ.get('DISPLAY') and os.environ['DISPLAY'].split('.')[0] != ':0'
assert not os.environ.get('WAYLAND_DISPLAY')
font = os.environ['CUI_TEST_FONT']
for mode in ('close', 'destroy', 'error', 'nested', 'resize'):
    app = App(backend='sdl', font_path=font, title='CUI Python callback runner')
    calls = []
    marker = ValueError('runner callback marker')
    def click(_event):
        calls.append(mode)
        if mode == 'resize': assert app.bounds(root)[2] > previous_width + 100
        if mode == 'error': raise marker
        if mode == 'nested':
            for operation in (app.run, app.step):
                try: operation()
                except RuntimeError: pass
                else: raise AssertionError('nested operation accepted')
        if mode == 'destroy': app.close()
        else: app.request_close()
    target = button('go').label('Go').on_click(click)
    try:
        root = root_view().label('CUI Python callback runner').children(row().children(target))
        app.mount(root); app.step()
        previous_width = app.bounds(root)[2]
        if mode == 'resize':
            window = subprocess.check_output(['xdotool', 'search', '--pid', str(os.getpid())], text=True, timeout=5).strip().splitlines()[0]
            subprocess.run(['xdotool', 'windowsize', '--sync', window, '720', '360'], check=True, timeout=5)
        x, y, _, _ = app.bounds(target)
        app.queue_pointer('down', x + 2, y + 2); app.queue_pointer('up', x + 2, y + 2)
        try: app.run()
        except ValueError as exc: assert mode == 'error' and exc is marker
        else: assert mode != 'error'
        assert calls == [mode]
        if mode != 'destroy':
            try: app.step()
            except BridgeError as exc: assert exc.code == 2
            else: raise AssertionError('runner did not terminate app')
    finally: app.close()
print('Python native callback run lifecycle PASS')
