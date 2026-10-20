#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Exercise the real WASM hex field, draft isolation and picker synchronization."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True)
            page = browser.new_page(viewport={'width': 1200, 'height': 1000}, device_scale_factor=2)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#ColorPicker')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="ColorPicker"')
            def click(name):
                page.locator('#preview').scroll_into_view_if_needed()
                target = page.evaluate('''name=>{const w=cuiGallery.exports,d=new TextDecoder();
                  for(let i=0;i<w.gallery_semantic_count();i++){
                    const text=d.decode(new Uint8Array(w.memory.buffer,w.gallery_semantic_name(i),w.gallery_semantic_length(i)));
                    if(text===name)return {x:w.gallery_semantic(i,1),y:w.gallery_semantic(i,2),width:w.gallery_semantic(i,3),height:w.gallery_semantic(i,4)};
                  }throw Error('Missing target '+name);}''', name)
                bounds = page.locator('#preview').bounding_box()
                page.evaluate('(y)=>window.scrollBy(0,y-innerHeight/2)', bounds['y']+target['y'])
                bounds = page.locator('#preview').bounding_box()
                page.mouse.click(bounds['x']+target['x']+target['width']/2,
                                 bounds['y']+target['y']+target['height']/2)
            def painted(text):
                expression = '''text=>{const w=cuiGallery.exports,d=new TextDecoder();
                  return Array.from({length:w.gallery_count()},(_,i)=>d.decode(new Uint8Array(w.memory.buffer,w.gallery_command_text(i),w.gallery_command_length(i)))).join('').includes(text);}'''
                try:
                    page.wait_for_function(expression, arg=text)
                except Exception:
                    print('Missing:', text, 'Errors:', errors, flush=True)
                    print(page.evaluate('''()=>{const w=cuiGallery.exports,d=new TextDecoder();
                      return Array.from({length:w.gallery_count()},(_,i)=>d.decode(new Uint8Array(w.memory.buffer,w.gallery_command_text(i),w.gallery_command_length(i)))).join('|');}'''), flush=True)
                    raise
            def replace(text, enabled=True):
                click('Hex color')
                if enabled:
                    page.wait_for_function('document.activeElement?.id==="cui-ime-input" && cuiGallery.exports.gallery_text_config(0)')
                page.keyboard.press('Control+a')
                page.keyboard.type(text)
            before = page.evaluate('cuiGallery.exports.gallery_color_rgba()>>>0')
            replace('#12')
            painted('incomplete draft')
            page.keyboard.press('Enter')
            assert page.evaluate('cuiGallery.exports.gallery_color_rgba()>>>0') == before
            painted('Hex commits: 0')
            replace('#12345678')
            page.keyboard.press('Enter')
            painted('Hex commits: 1')
            assert page.evaluate('cuiGallery.exports.gallery_color_rgba()>>>0') == 0x12345678
            page.keyboard.press('Enter')  # Equal-value commits remain observable.
            painted('Hex commits: 2')
            replace('#F')
            painted('incomplete draft')
            painted('#F')
            page.evaluate('cuiGallery.exports.gallery_screen_color(0xABCDEF);cuiGallery.render();')
            painted('#F')  # Dirty draft survives an external picker update.
            click('Use picker color')
            painted('#ABCDEF78')
            page.locator('#disabled').check()
            replace('#000000', enabled=False)
            page.keyboard.press('Enter')
            assert page.evaluate('cuiGallery.exports.gallery_color_rgba()>>>0') == 0xABCDEF78
            painted('Hex commits: 2')
            page.locator('#disabled').uncheck()
            page.set_viewport_size({'width': 390, 'height': 844})
            painted('Hex draft')
            assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
            assert not errors, errors
            browser.close()
            print('PASS: actual hex draft/commit, same-value commits, external picker conflict/restore, disabled and mobile')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
