#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual browser typing and IME through the gallery's whole-value name predicate."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

TEXTS = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,
 e.gallery_command_text(i),e.gallery_command_length(i))));}'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1',0), partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, **({'executable_path':args.chromium} if args.chromium else {}))
            audit = Audit(browser, f'http://127.0.0.1:{server.server_port}')
            audit.route('Input')
            audit.configure('input-mode',3)
            field = audit.target('Display name',6)
            point = audit.point(field,.25,.5)
            audit.page.mouse.click(point['x'],point['y'])
            audit.page.wait_for_function('document.activeElement.id === "cui-ime-input"')
            audit.page.keyboard.press('Control+a')
            audit.session.send('Input.insertText',{'text':'Ada'})
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===3')
            audit.session.send('Input.insertText',{'text':'7'})
            audit.page.wait_for_function('('+TEXTS+ ')().includes("Number rejected; your name is unchanged.")')
            assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 3
            assert 'Ada' in audit.texts()
            audit.session.send('Input.insertText',{'text':'é'})
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===5')
            assert 'Adaé' in audit.texts()
            assert 'Names can contain Unicode text, but no digits.' in audit.texts()
            # Native browser provisional text is validated without changing the committed document.
            audit.session.send('Input.imeSetComposition',{'text':'9','selectionStart':1,'selectionEnd':1})
            audit.page.wait_for_function('('+TEXTS+ ')().includes("Number rejected; your name is unchanged.")')
            assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 5
            audit.session.send('Input.imeSetComposition',{'text':'','selectionStart':0,'selectionEnd':0})
            audit.session.send('Input.insertText',{'text':'X'})
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===6')
            assert 'AdaéX' in audit.texts()
            # Legacy editable mode removes the example predicate and accepts the same digit.
            audit.configure('input-mode',0)
            audit.page.wait_for_function('!('+TEXTS+ ')().some(text => text.startsWith("Names can") || text.startsWith("Number rejected"))')
            field = audit.target('Display name',6)
            point = audit.point(field,.9,.5)
            audit.page.mouse.click(point['x'],point['y'])
            audit.page.wait_for_function('document.activeElement.id === "cui-ime-input"')
            audit.page.keyboard.press('End')
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(1)===cuiGallery.exports.gallery_input_selection(2)')
            audit.session.send('Input.insertText',{'text':'7'})
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===7')
            assert 'AdaéX7' in audit.texts()
            before_mobile = audit.page.evaluate('cuiGallery.frame')
            audit.page.set_viewport_size({'width':390,'height':900})
            audit.page.wait_for_function('(frame)=>cuiGallery.frame>frame',arg=before_mobile)
            audit.configure('input-mode',3)
            field = audit.target('Display name',6)
            point = audit.point(field,.5,.5)
            audit.page.mouse.click(point['x'],point['y'])
            audit.page.wait_for_function('document.activeElement.id === "cui-ime-input"')
            audit.page.keyboard.press('Control+a')
            audit.session.send('Input.insertText',{'text':'Ada'})
            audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===3')
            audit.session.send('Input.insertText',{'text':'7'})
            audit.page.wait_for_function('('+TEXTS+ ')().join(" ").includes("Number rejected;")')
            assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 3
            width = audit.page.locator('canvas').bounding_box()['width']
            painted = audit.page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),x:e.gallery_command(i,1),w:e.gallery_command(i,3)})).filter(c=>c.type===2);}''')
            assert all(c['x']>=0 and c['x']+c['w']<=width+.5 for c in painted),(width,painted)
            guides = audit.page.locator('#component-guides a').all_text_contents()
            assert any('acceptance' in g.lower() for g in guides),guides
            assert any('normalization' in g.lower() for g in guides),guides
            assert not audit.errors,audit.errors
            assert audit.page.evaluate('cuiGallery.exports.gallery_error()') == 0
            browser.close()
    finally:
        server.shutdown()
    print('Input actual gallery predicate typing, Unicode, IME rejection/cancel and guide links PASS')

if __name__ == '__main__':
    main()
