"""Opt-in SDL smoke proof; run only inside a disposable virtual display."""
import os
import re
from pathlib import Path
import subprocess
import tempfile
from cui_bridge import App, button, label, row

font = os.environ['CUI_TEST_FONT']
if (os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1' or os.environ.get('WAYLAND_DISPLAY')
        or not os.environ.get('DISPLAY') or os.environ.get('DISPLAY') == ':0'):
    raise RuntimeError('Native proof requires an isolated virtual display')
clicks = []
save = button('save').primary().rounded_md().hover_color('#ff0000').label('Save changes').on_click(
    lambda event: clicks.append(event.frame_sequence))
with App(backend='sdl', width=480, height=240, font_path=font,
         title='CUI Bridge Native Probe') as app:
    app.mount(row().gap_units(2).children(save, label('Ready ✓')))
    app.step()
    x, y, w, h = app.bounds(save)
    assert w > 0 and h > 0
    tree = subprocess.check_output(['xwininfo', '-root', '-tree'], text=True)
    matches = re.findall(r'(0x[0-9a-f]+) \"CUI Bridge Native Probe\"', tree)
    assert len(matches) == 1, tree
    window = matches[0]
    def pixel_at(px, py):
        with tempfile.TemporaryDirectory() as temp:
            capture = Path(temp) / 'native.xwd'
            subprocess.run(['xwd', '-silent', '-id', window, '-out', str(capture)], check=True)
            assert capture.stat().st_size > 10000
            return subprocess.check_output(['convert', str(capture), '-format',
                                            f'%[pixel:p{{{px},{py}}}]', 'info:'],
                                           text=True).strip()
    sample_x, sample_y = int(x + 4), int(y + 4)
    normal = pixel_at(sample_x, sample_y)
    assert normal and normal not in ('srgb(0,0,0)', 'srgb(255,255,255)'), normal
    subprocess.run(['xdotool', 'mousemove', '--window', window,
                    str(int(x + w / 2)), str(int(y + h / 2))], check=True)
    app.step()
    hover = pixel_at(sample_x, sample_y)
    assert hover != normal, (normal, hover)
    with tempfile.TemporaryDirectory() as temp:
        capture = Path(temp) / 'native.xwd'
        subprocess.run(['xwd', '-silent', '-id', window, '-out', str(capture)], check=True)
        assert capture.stat().st_size > 10000
        pixel = subprocess.check_output(['convert', str(capture), '-format',
                                         f'%[pixel:p{{{int(x+w/2)},{int(y+h/2)}}}]',
                                         'info:'], text=True).strip()
        assert pixel and pixel not in ('srgb(0,0,0)', 'srgb(255,255,255)'), pixel
    subprocess.run(['xdotool', 'mousemove', '--window', window,
                    str(int(x + w / 2)), str(int(y + h / 2)), 'click', '1'], check=True)
    app.step()
    assert app.dispatch() == 1 and len(clicks) == 1, clicks
print('native virtual SDL render, framebuffer pixel, real X11 click PASS')
