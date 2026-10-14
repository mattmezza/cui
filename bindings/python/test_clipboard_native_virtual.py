"""Native clipboard proof restricted to a disposable X11 display."""
import os
from cui_bridge import App,root_view,label
if os.environ.get('CUI_TEST_VIRTUAL_DISPLAY')!='1' or os.environ.get('WAYLAND_DISPLAY') or not os.environ.get('DISPLAY') or os.environ['DISPLAY']==':0':
    raise RuntimeError('Requires isolated virtual display')
with App(backend='sdl',font_path=os.environ['CUI_TEST_FONT']) as app:
    app.mount(root_view().children(label('Clipboard round trip')));app.step()
    for text in ('Unicode ✓ é', 'é'*32768, ''):
        app.clipboard_set(text);assert app.clipboard_get()==text
print('PASS Python SDL clipboard Unicode/64KiB/empty round trips')
