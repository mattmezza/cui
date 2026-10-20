#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM month menu and independently selectable neighboring dates."""
import argparse
from datetime import datetime, timezone
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
            page.clock.set_fixed_time(datetime(2026, 10, 20, tzinfo=timezone.utc))
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#Calendar')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Calendar"')
            target_js = '''name=>{const w=cuiGallery.exports,d=new TextDecoder();
              for(let i=0;i<w.gallery_semantic_count();i++){
                const text=d.decode(new Uint8Array(w.memory.buffer,w.gallery_semantic_name(i),w.gallery_semantic_length(i)));
                if(text===name)return {i,x:w.gallery_semantic(i,1),y:w.gallery_semantic(i,2),width:w.gallery_semantic(i,3),height:w.gallery_semantic(i,4)};
              }return null;}'''
            def target(name):
                page.wait_for_function('(name)=>('+target_js+')(name)!==null', arg=name)
                return page.evaluate(target_js, name)
            def click(name):
                t = target(name)
                b = page.locator('#preview').bounding_box()
                page.evaluate('(y)=>window.scrollBy(0,y-innerHeight/2)', b['y']+t['y'])
                b = page.locator('#preview').bounding_box()
                frame = page.evaluate('cuiGallery.frame')
                page.mouse.click(b['x']+t['x']+t['width']/2, b['y']+t['y']+t['height']/2)
                page.wait_for_function('(frame)=>cuiGallery.frame>frame', arg=frame)
            def cell_color(name):
                return page.evaluate('''b=>{const w=cuiGallery.exports;
                  for(let i=0;i<w.gallery_count();i++){
                    const f=n=>w.gallery_command(i,n);
                    if(f(0)===0&&Math.abs(f(1)-b.x)<.1&&Math.abs(f(2)-b.y)<.1&&Math.abs(f(3)-b.width)<.1)
                      return [f(5),f(6),f(7),f(8)];
                  }throw Error('Missing cell paint');}''', target(name))
            ordinary = cell_color('2026-10-20')
            click('Mark today')
            assert cell_color('2026-10-20') != ordinary
            assert page.evaluate('cuiGallery.exports.gallery_date_value()') == 20261012
            assert page.evaluate(target_js, '2026-09-28') is None
            click('Adjacent days')
            click('2026-09-28')
            page.wait_for_function('cuiGallery.exports.gallery_date_value()===20260928')
            click('Month menu')
            click('Choose month')
            click('February')
            target('2026-02-02')
            assert page.evaluate('cuiGallery.exports.gallery_date_value()') == 20260928
            page.locator('#disabled').check()
            click('2026-02-02')
            assert page.evaluate('cuiGallery.exports.gallery_date_value()') == 20260928
            page.locator('#disabled').uncheck()
            click('2026-02-02')
            page.wait_for_function('cuiGallery.exports.gallery_date_value()===20260202')
            click('Mark today')
            frame = page.evaluate('cuiGallery.frame')
            page.set_viewport_size({'width': 390, 'height': 844})
            page.wait_for_function('(frame)=>cuiGallery.frame>frame', arg=frame)
            controls = [target(name) for name in ('Month menu', 'Adjacent days', 'Mark today')]
            assert controls[0]['y'] < controls[1]['y'] < controls[2]['y']
            assert all(t['width'] > 0 and t['height'] > 0 for t in controls)
            assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
            assert not errors, errors
            browser.close()
            print('PASS: month menu, neighboring-date selection/recenter, disabled suppression and responsive control layout')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
