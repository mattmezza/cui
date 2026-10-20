#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM shortcut composition and owner-validated Stepper requests."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 1000}, device_scale_factor=2)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#Kbd')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Kbd"')
            target_js = '''name=>{const w=cuiGallery.exports,d=new TextDecoder();
              for(let i=0;i<w.gallery_semantic_count();i++){
                const text=d.decode(new Uint8Array(w.memory.buffer,w.gallery_semantic_name(i),w.gallery_semantic_length(i)));
                if(text===name)return {i,x:w.gallery_semantic(i,1),y:w.gallery_semantic(i,2),width:w.gallery_semantic(i,3),height:w.gallery_semantic(i,4)};
              }throw Error('Missing target '+name);}'''
            def target(name):
                return page.evaluate(target_js, name)
            hints = [target(name) for name in ('Ctrl+Shift+K', 'Esc', 'Enter')]
            assert all(h['width'] > 0 and h['height'] > 0 for h in hints)
            assert hints[0]['x'] < hints[1]['x'] < hints[2]['x']
            assert abs(hints[0]['y']-hints[1]['y']) < 2
            assert 'cui_kbd_ex' in page.locator('#snippet').inner_text()
            assert page.locator('#component-guides a').filter(has_text='Keyboard').count() >= 1
            page.goto(f'http://127.0.0.1:{server.server_port}/#Stepper')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Stepper"')
            def painted(text):
                expression = """()=>{const w=cuiGallery.exports,d=new TextDecoder();
                  return Array.from({length:w.gallery_count()},(_,i)=>d.decode(new Uint8Array(w.memory.buffer,w.gallery_command_text(i),w.gallery_command_length(i)))).join('');}"""
                try:
                    page.wait_for_function('(text)=>('+expression+')().includes(text)', arg=text, timeout=10000)
                except Exception:
                    print('Missing paint:', text, 'Actual:', page.evaluate(expression)[:1200], 'Errors:', errors, flush=True)
                    raise
            def click(name):
                t = target(name)
                b = page.locator('#preview').bounding_box()
                page.mouse.click(b['x']+t['x']+t['width']/2, b['y']+t['y']+t['height']/2)
            painted('Step 1 of 3')
            click('Review')
            painted('Confirm details to continue.')
            painted('Navigation requests: 1')
            painted('Step 1 of 3')
            click('Finish')
            painted('Navigation requests: 2')
            painted('Step 1 of 3')
            click('Details')
            painted('Navigation requests: 3')  # Same selected item still requests.
            click('Details confirmed')
            click('Review')
            painted('Step 2 of 3')
            painted('Navigation requests: 4')
            click('Finish')
            painted('Step 3 of 3')
            click('Finish')
            painted('Navigation requests: 6')
            page.locator('#disabled').check()
            click('Details')
            painted('Step 3 of 3')
            painted('Navigation requests: 6')
            page.locator('#disabled').uncheck()
            page.evaluate('(i)=>{cuiGallery.exports.gallery_focus(i);cuiGallery.render();}', target('Review')['i'])
            page.locator('#preview').focus()
            page.keyboard.press('Enter')
            painted('Step 2 of 3')
            painted('Navigation requests: 7')
            assert 'cui_stepper_navigation' in page.locator('#snippet').inner_text()
            frame = page.evaluate('cuiGallery.frame')
            page.set_viewport_size({'width': 390, 'height': 844})
            page.wait_for_function('(frame)=>cuiGallery.frame>frame', arg=frame)
            painted('Your app validates each request.')
            assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
            assert not errors, errors
            browser.close()
            print('PASS: typed Kbd row and actual pointer/key Stepper validation, same-item requests, disabled suppression and mobile containment')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
