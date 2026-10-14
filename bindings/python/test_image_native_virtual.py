"""Owned image pixels and font fallbacks on an isolated SDL framebuffer."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from cui_bridge import App, column, image, input

if (os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1' or os.environ.get('WAYLAND_DISPLAY')
        or not os.environ.get('DISPLAY') or os.environ.get('DISPLAY') == ':0'):
    raise RuntimeError('Native proof requires an isolated virtual display')
font=os.environ['CUI_TEST_FONT']
pixels=bytearray([255,0,0,255]*4)
photo=image('image').image_rgba(pixels,2,2).visual_size(80,80)
pixels.clear()
with App(backend='sdl',font_path=font,width=240,height=160,title='CUI Owned Image Probe') as app:
    app.mount(column().children(photo));app.step()
    tree=subprocess.check_output(['xwininfo','-root','-tree'],text=True)
    windows=re.findall(r'(0x[0-9a-f]+) \"CUI Owned Image Probe\"',tree)
    assert len(windows)==1,tree
    x,y,w,h=app.bounds(photo)
    def pixel():
        with tempfile.TemporaryDirectory() as temp:
            capture=Path(temp)/'native.xwd'
            subprocess.run(['xwd','-silent','-id',windows[0],'-out',str(capture)],check=True)
            return subprocess.check_output(['convert',str(capture),'-format',f'%[pixel:p{{{int(x+w/2)},{int(y+h/2)}}}]','info:'],text=True).strip()
    assert pixel()=='srgb(255,0,0)',pixel()
    photo.image_rgba(bytes([0,255,0,255]*4),2,2);app.step()
    assert pixel()=='srgb(0,255,0)',pixel()
fallback=os.environ.get('CUI_TEST_FALLBACK_FONT')
if fallback:
    text=input('input','Ready ✓')
    with App(backend='sdl',font_path=font,width=240,height=160,title='CUI Font Fallback Probe') as app:
        app.font_fallbacks([fallback]).mount(column().children(text));app.step()
        try: app.font_fallbacks([fallback+'.missing'])
        except Exception: pass
        else: raise AssertionError('missing fallback accepted')
        app.step()  # A failed replacement preserves the preceding owned chain.
        assert text.current_text()=='Ready ✓'
print('PASS actual SDL copied/replaced image pixels and optional Unicode fallback')
